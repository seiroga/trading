#pragma once

#include <core/data_storage.h>
#include <core/connector.h>
#include <core/strategy.h>

#include <boost/signals2.hpp>

namespace tbp
{
	namespace oanda
	{
		class trader : public sb::dynamic
		{
			class trading_db;

		private:
			const data_provider::ptr m_data_provider;
			const tbp::connector::ptr m_connector;
			const strategy::ptr m_strategy;
			const std::unique_ptr<trading_db> m_db;

			boost::signals2::connection m_on_open_trade_connection;
			boost::signals2::connection m_on_close_trade_connection;

		private:
			void on_open_trade(const std::wstring& instrument_id, long amount, strategy::trade_id_t internal_id);
			void on_close_trade(strategy::trade_id_t internal_id, long amount);

			void update_objects_states();
			void cancel_all_pending_tasks();

		public:
			trader(const tbp::connector::ptr& c, const strategy::ptr& st, const std::wstring& working_dir);
			~trader();
		};
	}
}