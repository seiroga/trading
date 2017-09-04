#include "data_collector_impl.h"

namespace tbp
{
	void data_collector_impl::collect_data_thread()
	{
		auto res = win::wait_for_multiple_objects(false, INFINITE, m_start_evt, m_stop_evt);
		if (1 == res.second)
		{
			// SB: stop request
			return;
		}

		while (m_stop_evt.wait(100))
		{
			auto data = m_connector->get_instrument_data(m_instrument_id);
			m_data_storage->save_instrument_data(m_instrument_id, { data });
		}
	}

	data_provider::ptr data_collector_impl::get_data_provider()
	{
		return m_data_storage;
	}

	data_collector_impl::data_collector_impl(const std::wstring& instrument_id, const tbp::connector::ptr& connector, const data_storage::ptr& ds)
		: m_start_evt(true, false)
		, m_stop_evt(true, false)
		, m_instrument_id(instrument_id)
		, m_data_storage(ds)
		, m_connector(connector)
		, m_worker(std::bind(&data_collector_impl::collect_data_thread, this))
	{
	}

	data_collector_impl::~data_collector_impl()
	{
		m_stop_evt.set();
		m_worker.join();
	}
}