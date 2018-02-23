#pragma once

#include <core/data_storage.h>
#include <sqlite/sqlite.h>

namespace tbp
{
	namespace oanda
	{
		namespace values
		{
			namespace instrument_data
			{
				extern const std::wstring c_timestamp;
				extern const std::wstring c_volume;
				extern const std::wstring c_bid_candlestick;
				extern const std::wstring c_ask_candlestick;
				extern const std::wstring c_complete;
			}

			namespace instant_data
			{
				extern const std::wstring c_bid_price;
				extern const std::wstring c_ask_price;
			}

			namespace candlestick_data
			{
				extern const std::wstring c_open_price;
				extern const std::wstring c_high_price;
				extern const std::wstring c_low_price;
				extern const std::wstring c_close_price;
			}
		}

		class data_storage : public tbp::data_storage
		{
			const sqlite::connection::ptr m_db;

		private:
			void create_db_schema();
			void verify_db_schema();

			__int64 get_instrument_row_id(const std::wstring& instrument_id);

		public:
			virtual std::vector<data_t::ptr> get_data(const std::wstring& instrument_id, unsigned long granularity, time_t* start_datetime, time_t* end_datetime) const override;
			virtual std::vector<data_t::ptr> get_instant_data(const std::wstring& instrument_id, time_t* start_datetime, time_t* end_datetime) const override;
			virtual void save_data(const std::wstring& instrument_id, unsigned long granularity, const std::vector<data_t::ptr>& data) override;
			virtual void save_instant_data(const std::wstring& instrument_id, const std::vector<data_t::ptr>& data) override;

		public:
			data_storage(const sqlite::connection::ptr& db);
		};
	}
}