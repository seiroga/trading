#include <oanda/data_storage.h>
#include <logging/log.h>

namespace tbp
{
	namespace oanda
	{
		namespace
		{
			const auto current_schema_version = 1;
		}

		void data_storage::create_db_schema()
		{
			auto version = m_db->schema_version();
			switch (version)
			{
			case 0:
				break;

			case current_schema_version:
				return;

			default:
				throw std::runtime_error("Unknown data storage DB schema version!");
			}

			LOG_INFO << "Oanda data storage DB schema is empty. Creating DB chema!";

			m_db->create_statement(L"CREATE TABLE INSTRUMENTS(ID INTEGER, NAME TEXT, PRIMRY KEY(ID))");
			m_db->create_statement(L"CREATE TABLE CANDLES(ID INTEGER, O_PRICE REAL, H_PRICE REAL, L_PRICE REAL, C_PRICE REAL, PRIMRY KEY(ID))");
			m_db->create_statement(L"CREATE TABLE INSTRUMENT_DATA(ID INTEGER, TIMESTAMP INTEGER, BID_CANDLESTICK_ID INTEGER, ASK_CANDLESTICK_ID INTEGER, VOLUME INTEGER, FOREIGN KEY(ID) REFERENCES INSTRUMENTS(ID), FOREIGN KEY(BID_CANDLESTICK_ID) REFERENCES CANDLES(ID), FOREIGN KEY(ASK_CANDLESTICK_ID) REFERENCES CANDLES(ID))");

			m_db->set_schema_version(current_schema_version);

			LOG_INFO << "Oanda data storage DB schema has been created successfully!";
		}

		void data_storage::verify_db_schema()
		{

		}

		std::vector<data_t::ptr> data_storage::get_instrument_data(const std::wstring& instrument_id, time_t start_datetime, time_t end_datetime)
		{

		}

		void data_storage::save_instrument_data(const std::wstring& instrument_id, const std::vector<data_t::ptr>& data)
		{

		}

		data_storage::data_storage(const sqlite::connection::ptr& db)
			: m_db(db)
		{
			create_db_schema();
		}
	}
}