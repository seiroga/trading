#pragma once

#include <core/data_storage.h>
#include <core/connector.h>
#include <core/trader.h>

namespace tbp
{
	namespace oanda
	{
		class trader : public tbp::trader
		{
			class trading_db;

		private:
			const data_provider::ptr m_data_provider;
			const tbp::connector::ptr m_connector;
			const std::unique_ptr<trading_db> m_db;

		private:
			void update_objects_states();

		public:
			virtual std::wstring open_trade(const std::wstring& instrument_id, double amount) override;
			virtual void close_trade(const std::wstring& internal_id, double amount) override;
			virtual void close_pending_trades() override;

			virtual std::vector<candlestick_data> get_candles_from_data(const std::vector<data_t::ptr>& candles_data) const override;

		public:
			trader(const tbp::connector::ptr& c, const std::wstring& working_dir);
			~trader();
		};
	}
}