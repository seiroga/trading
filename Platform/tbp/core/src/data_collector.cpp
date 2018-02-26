#include <core/data_collector.h>
#include <core/utilities.h>

#include <logging/log.h>

#include <boost/numeric/conversion/cast.hpp>

#include <future>
#include <sstream>

namespace tbp
{
	namespace
	{
		// SB: this code is taken from oanda connector implementation for testing purposes
		std::wstring to_str(time_t time)
		{
			const auto epoch_time_t = std::chrono::system_clock::to_time_t(time_t());
			auto epoch_time = gmtime(&epoch_time_t);
			auto raw_time = time.time_since_epoch().count();
			SYSTEMTIME st = { 0 };
			if (0 == ::FileTimeToSystemTime(reinterpret_cast<const FILETIME*>(&raw_time), &st))
			{
				throw win::exception(L"FileTimeToSystemTime call failed!");
			}

			// according to https://tools.ietf.org/rfc/rfc3339.txt
			return std::to_wstring(st.wYear - 1601 + epoch_time->tm_year + 1900) + L"-" + std::to_wstring(st.wMonth) + L"-" + std::to_wstring(st.wDay) + L"T" + std::to_wstring(st.wHour) + L":" +
				std::to_wstring(st.wMinute) + L":" + std::to_wstring(st.wSecond) + (0 != st.wMilliseconds ? L"." + std::to_wstring(st.wMilliseconds) : L".000000000") + L"Z";
		}

		unsigned long get_millisecs_delay_till_next_request(std::chrono::seconds granularity)
		{
			auto curr_time = tbp::time_t::clock::now();
			auto aligned_next_time = align_to_granularity<tbp::time_t::duration>(curr_time, granularity) + granularity;
			auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(aligned_next_time - curr_time.time_since_epoch());
			
			// SB: this additional delay needed to compensate inaccuracy of system timer and 
			// inaccuracy during arithmetic operations of time values with different precision (nanoseconds / milliseconds / seconds)
			// Example of this situation provided below:
			// [2018-02-04 16:42:17.001106][0x000053c0][info]    Curr time: 14:42:16.999Z. Aligned next time: 14:42:17.000000000Z. Diff ms : 0
			// [2018-02-04 16:42:17.001106][0x000053c0][info]    Curr time: 14:42:17.1Z. Aligned next time: 14:42:18.000000000Z. Diff ms : 998
			// [2018-02-04 16:42:18.000273][0x000053c0][info]    Curr time: 14:42:17.999Z. Aligned next time: 14:42:18.000000000Z. Diff ms : 0
			// [2018-02-04 16:42:18.001273][0x000053c0][info]    Curr time: 14:42:18.1Z. Aligned next time: 14:42:19.000000000Z. Diff ms : 998
			// [2018-02-04 16:42:19.000595][0x000053c0][info]    Curr time: 14:42:18.999Z. Aligned next time: 14:42:19.000000000Z. Diff ms : 0
			// [2018-02-04 16:42:19.001592][0x000053c0][info]    Curr time: 14:42:19.1Z. Aligned next time: 14:42:20.000000000Z. Diff ms : 998
			// [2018-02-04 16:42:19.999606][0x000053c0][info]    Curr time: 14:42:19.999Z. Aligned next time: 14:42:20.000000000Z. Diff ms : 0
			// [2018-02-04 16:42:20.000548][0x000053c0][info]    Curr time: 14:42:20.000000000Z. Aligned next time: 14:42:21.000000000Z. Diff ms : 999
			
			diff += std::chrono::milliseconds(100);

			//LOG_INFO << L"Curr time: " << to_str(curr_time) << L". Aligned next time: " << to_str(tbp::time_t(std::chrono::duration_cast<tbp::time_t::duration>(aligned_next_time))) << L". Diff ms: " << diff.count();

			return boost::numeric_cast<unsigned long>(diff.count());
		}
	}

	void data_collector::collect_instant_data_thread()
	{
		// SB: temporary disable instant data collecting
		return;

		auto res = win::wait_for_multiple_objects(false, INFINITE, m_start_evt, m_stop_evt);
		if (1 == res.second)
		{
			// SB: stop request
			return;
		}

		while (!m_stop_evt.wait(10))
		{
			try
			{
				auto data = m_connector->get_instant_data(m_instrument_id);

				{
					win::scoped_lock lock(m_cache_cs);

					m_cache.emplace_back(std::move(data));

					if (m_cache.size() >= m_cache_size)
					{
						// SB: need to think when to call this signal, on each 100 vaues in cache or on each new data from server
						on_instant_data(m_instrument_id, m_cache);

						flush_cache();
					}
				}
			}
			catch (const tbp::http_exception& ex)
			{
				LOG_ERR << L"Exception was thrown during HTTP request. Code: " << ex.code << L" Info: " << ex.what();
			}
			catch (const std::exception& ex)
			{
				LOG_ERR << L"Exception was thrown during retrieving data from server." << L" Info: " << ex.what();
			}
			catch (...)
			{
				LOG_ERR << L"Unknown error! Exception was thrown during retrieving data from server!";
			}
		}

		// SB: flush all newly collected data
		flush_cache();
	}

	void data_collector::collect_historical_data_thread()
	{
		auto res = win::wait_for_multiple_objects(false, INFINITE, m_start_evt, m_stop_evt);
		if (1 == res.second)
		{
			// SB: stop request
			return;
		}

		// SB: lets synchronize our request with data granularity
		const std::chrono::seconds granularity_secs(m_historcial_data_granularity);
		unsigned long wait_interval = get_millisecs_delay_till_next_request(granularity_secs);
		while (!m_stop_evt.wait(wait_interval))
		{
			try
			{
				auto curr_time = tbp::time_t::clock::now();
				curr_time = tbp::time_t(align_to_granularity<tbp::time_t::duration>(curr_time, granularity_secs));
				auto start_time = curr_time - granularity_secs;
				auto data = m_connector->get_data(m_instrument_id, m_historcial_data_granularity, &start_time, nullptr);
				if (!data.empty())
				{
					m_data_storage->save_data(m_instrument_id, m_historcial_data_granularity, data);
					on_historical_data(m_instrument_id, data);
				}
				else
				{
					// SB: strange case but we need to retry
					LOG_DBG << L"Connector has returned empty data on get historical data request!";

					wait_interval = 1000;
					continue;
				}
			}
			catch (const tbp::http_exception& ex)
			{
				LOG_ERR << L"Exception was thrown during HTTP request. Code: " << ex.code << L" Info: " << ex.what();

				wait_interval = 1000;
				continue;
			}
			catch (const std::exception& ex)
			{
				LOG_ERR << L"Exception was thrown during retrieving data from server." << L" Info: " << ex.what();
			}
			catch (...)
			{
				LOG_ERR << L"Unknown error! Exception was thrown during retrieving data from server!";
			}

			wait_interval = get_millisecs_delay_till_next_request(granularity_secs);
		}
	}

	void data_collector::flush_cache()
	{
		win::scoped_lock lock(m_cache_cs);

		if (m_cache.empty())
		{
			return;
		}

		m_data_storage->save_instant_data(m_instrument_id, m_cache);
		m_cache.clear();
	}

	std::vector<data_t::ptr> data_collector::get_data(const std::wstring& instrument_id, unsigned long granularity, time_t* start_datetime, time_t* end_datetime) const
	{
		if (nullptr == start_datetime || nullptr == end_datetime)
		{
			throw std::invalid_argument("start_datetime or end_datetime argument is null!");
		}

		auto actual_start = *start_datetime;
		auto actual_end = *end_datetime;
		auto result = m_data_storage->get_data(instrument_id, granularity, &actual_start, &actual_end);

		if (actual_start != *start_datetime || actual_end != *end_datetime)
		{
			// SB: should we make connector call from worker thread? Is it some reason for this? Anyway we will wait untill data arrives...
			result = m_connector->get_data(instrument_id, granularity, start_datetime, end_datetime);
			m_data_storage->save_data(instrument_id, granularity, result);
		}

		return result;
	}

	std::vector<data_t::ptr> data_collector::get_instant_data(const std::wstring& instrument_id, time_t* start_datetime, time_t* end_datetime) const
	{
		if (nullptr == start_datetime || nullptr == end_datetime)
		{
			throw std::invalid_argument("start_datetime or end_datetime argument is null!");
		}

		const_cast<data_collector*>(this)->flush_cache();

		auto actual_start = *start_datetime;
		auto actual_end = *end_datetime;
		auto result = m_data_storage->get_instant_data(instrument_id, &actual_start, &actual_end);

		return result;
	}

	void data_collector::start()
	{
		m_start_evt.set();

		LOG_DBG << L"Data collector has been started!";
	}

	data_collector::data_collector(const std::wstring& instrument_id, const settings::ptr& s, const tbp::connector::ptr& connector, const data_storage::ptr& ds)
		: m_cache_size(get_value<int>(s, L"DataCollectorCacheSize", 100))
		, m_historcial_data_granularity(get_value<int>(s, L"DataGranularity", 60))
		, m_start_evt(true, false)
		, m_stop_evt(true, false)
		, m_instrument_id(instrument_id)
		, m_data_storage(ds)
		, m_connector(connector)
		, m_instant_worker(std::bind(&data_collector::collect_instant_data_thread, this))
		, m_historical_worker(std::bind(&data_collector::collect_historical_data_thread, this))
	{
	}

	data_collector::~data_collector()
	{
		m_stop_evt.set();
		m_instant_worker.join();
		m_historical_worker.join();

		LOG_DBG << L"Data collector has been shutdown!";
	}
}