#pragma once

#include <core/data_storage.h>
#include <sqlite/sqlite.h>

namespace tbp
{
	namespace oanda
	{
		class data_storage : public tbp::data_storage
		{
			const sqlite::connection::ptr m_db;

		private:
			void create_db_schema();
			void verify_db_schema();

		public:
			virtual std::vector<data_t::ptr> get_instrument_data(const std::wstring& instrument_id, time_t start_datetime, time_t end_datetime) override;
			virtual void save_instrument_data(const std::wstring& instrument_id, const std::vector<data_t::ptr>& data) override;

		public:
			data_storage(const sqlite::connection::ptr& db);
		};
	}
}