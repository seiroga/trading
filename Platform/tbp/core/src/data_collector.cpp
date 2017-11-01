#include <core/data_collector.h>

#include <logging/log.h>

#include <future>

namespace tbp
{
	void data_collector::collect_data_thread()
	{
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

	std::vector<data_t::ptr> data_collector::get_data(const std::wstring& instrument_id, time_t* start_datetime, time_t* end_datetime) const
	{
		if (nullptr == start_datetime || nullptr == end_datetime)
		{
			throw std::invalid_argument("start_datetime or end_datetime argument is null!");
		}

		auto actual_start = *start_datetime;
		auto actual_end = *end_datetime;
		auto result = m_data_storage->get_data(instrument_id, &actual_start, &actual_end);

		if (actual_start != *start_datetime || actual_end != *end_datetime)
		{
			// SB: should we make connector call from worker thread? Is it some reason for this? Anyway we will wait untill data arrives...
			result = m_connector->get_data(instrument_id, start_datetime, end_datetime);
			m_data_storage->save_data(instrument_id, result);
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

	data_collector::data_collector(const std::wstring& instrument_id, const settings::ptr& s, const tbp::connector::ptr& connector, const data_storage::ptr& ds)
		: m_cache_size(get_value<int>(s, L"DataCollectorCacheSize", 100))
		, m_start_evt(true, false)
		, m_stop_evt(true, false)
		, m_instrument_id(instrument_id)
		, m_data_storage(ds)
		, m_connector(connector)
		, m_worker(std::bind(&data_collector::collect_data_thread, this))
	{
		m_start_evt.set();
	}

	data_collector::~data_collector()
	{
		m_stop_evt.set();
		m_worker.join();
	}
}