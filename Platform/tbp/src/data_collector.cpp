#include "data_collector.h"

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