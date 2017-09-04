#include "data_collector.h"

namespace tbp
{
	namespace
	{
		class data_provider_impl : public tbp::data_provider
		{
		public:
			virtual std::vector<data_t::ptr> get_instrument_data(const std::wstring& instrument_id, time_t start_datetime, time_t end_datetime) override
			{
				return {};
			}
		};
	}

	void data_collector::collect_data_thread()
	{
		auto res = win::wait_for_multiple_objects(false, INFINITE, m_start_evt, m_stop_evt);
		if (1 == res.second)
		{
			// SB: stop request
			return;
		}

		while (m_stop_evt.wait(200))
		{
			auto data = m_connector->get_instrument_data(m_instrument_id);
		}
	}

	data_provider::ptr data_collector::create_data_provider()
	{
		return std::make_shared<data_provider_impl>();
	}

	data_collector::data_collector(const std::wstring& instrument_id, const tbp::connector::ptr& connector, const std::wstring& db_path)
		: m_start_evt(true, false)
		, m_stop_evt(true, false)
		, m_instrument_id(instrument_id)
		, m_db(sqlite::connection::create(db_path))
		, m_connector(connector)
		, m_worker(std::bind(&data_collector::collect_data_thread, this))
	{
	}
}