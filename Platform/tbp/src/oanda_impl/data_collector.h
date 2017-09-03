#pragma once

#include <tbp/data_collector.h>
#include <tbp/connector.h>

#include <sqlite/sqlite.h>

#include<win/thread.h>

#include <thread>
#include <string>

namespace tbp
{
	namespace oanda
	{
		//SB: need to move to the common tbp code
		class data_collector : public tbp::data_collector
		{
			win::event m_start_evt;
			win::event m_stop_evt;
			const std::wstring m_instrument_id;
			const sqlite::connection::ptr m_db;
			const tbp::connector::ptr m_connector;
			std::vector<data_t> m_cache;
			std::thread m_worker;

		private:
			void collect_data_thread();

		public:
			virtual data_provider::ptr create_data_provider() override;

		public:
			data_collector(const std::wstring& instrument_id, const tbp::connector::ptr& connector, const std::wstring& db_path);
		};
	}
}