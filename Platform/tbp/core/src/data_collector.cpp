#include <core/data_collector.h>

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

		while (!m_stop_evt.wait(100))
		{
			auto data = m_connector->get_instrument_data(m_instrument_id);
			m_data_storage->save_instrument_data(m_instrument_id, { data });
		}
	}

	std::vector<data_t::ptr> data_collector::get_instrument_data(const std::wstring& instrument_id, time_t* start_datetime, time_t* end_datetime) const
	{
		if (nullptr == start_datetime || nullptr == end_datetime)
		{
			throw std::invalid_argument("start_datetime or end_datetime argument is null!");
		}

		auto actual_start = *start_datetime;
		auto actual_end = *end_datetime;
		auto result = m_data_storage->get_instrument_data(instrument_id, &actual_start, &actual_end);

		if (actual_start != *start_datetime || actual_end != *end_datetime)
		{
			// SB: should we make connector call from worker thread? Is it some reason for this? Anyway we will wait untill data arrives...
			result = m_connector->get_instrument_data(instrument_id, start_datetime, end_datetime);
			m_data_storage->save_instrument_data(instrument_id, result);
		}

		return result;
	}

	data_collector::data_collector(const std::wstring& instrument_id, const tbp::connector::ptr& connector, const data_storage::ptr& ds)
		: m_start_evt(true, false)
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