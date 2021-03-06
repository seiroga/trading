#include <oanda/data_storage.h>
#include <logging/log.h>

namespace tbp
{
	namespace oanda
	{
		namespace values
		{
			namespace instrument_data
			{
				const std::wstring c_timestamp(L"TIMESTAMP");
				const std::wstring c_volume(L"VOLUME");
				const std::wstring c_bid_candlestick(L"BID_CANDLESTICK");
				const std::wstring c_ask_candlestick(L"ASK_CANDLESTICK");
				const std::wstring c_complete(L"COMPLETE");
			}

			namespace instant_data
			{
				const std::wstring c_bid_price(L"BID");
				const std::wstring c_ask_price(L"ASK");
			}

			namespace candlestick_data
			{
				const std::wstring c_open_price(L"O_PRICE");
				const std::wstring c_high_price(L"H_PRICE");
				const std::wstring c_low_price(L"L_PRICE");
				const std::wstring c_close_price(L"C_PRICE");
			}
		}

		namespace
		{
			const auto current_schema_version = 1;

			tbp::data_t read_candelstick_data(const std::shared_ptr<sqlite::statement>& candels_data_st)
			{
				tbp::data_t candelstick_data;

				candelstick_data[values::candlestick_data::c_open_price] = candels_data_st->get_value<double>(0);
				candelstick_data[values::candlestick_data::c_high_price] = candels_data_st->get_value<double>(1);
				candelstick_data[values::candlestick_data::c_low_price] = candels_data_st->get_value<double>(2);
				candelstick_data[values::candlestick_data::c_close_price] = candels_data_st->get_value<double>(3);

				return candelstick_data;
			}
		}

		void data_storage::create_db_schema()
		{
			const auto version = m_db->schema_version();
			switch (version)
			{
			case 0:
				break;

			case current_schema_version:
				return;

			default:
				throw std::runtime_error("Unknown data storage DB schema version!");
			}

			sqlite::transaction t(m_db);

			try
			{
				LOG_INFO << "Oanda data storage DB schema is empty. Creating DB chema!";

				auto st = m_db->create_statement(L"CREATE TABLE [INSTRUMENTS]([ID] INTEGER PRIMARY KEY NOT NULL, [NAME] TEXT)");
				st->step();

				st = m_db->create_statement(L"CREATE TABLE [CANDLES]([ID] INTEGER PRIMARY KEY NOT NULL, [O_PRICE] DOUBLE, [H_PRICE] DOUBLE, [L_PRICE] DOUBLE, [C_PRICE] DOUBLE)");
				st->step();

				st = m_db->create_statement(L"CREATE TABLE [INSTRUMENT_DATA]([INSTRUMENT_ID] REFERENCES INSTRUMENTS(ID) ON DELETE CASCADE, [TIMESTAMP] INTEGER, [GRANULARITY] INTEGER, [BID_CANDLESTICK] REFERENCES CANDLES(ID), [ASK_CANDLESTICK] REFERENCES CANDLES(ID), [VOLUME] INTEGER, PRIMARY KEY([INSTRUMENT_ID], [TIMESTAMP], [GRANULARITY]))");
				st->step();

				st = m_db->create_statement(L"CREATE TABLE [INSTANT_INSTRUMENT_DATA]([INSTRUMENT_ID] REFERENCES INSTRUMENTS(ID) ON DELETE CASCADE, [TIMESTAMP] INTEGER, [BID] DOUBLE, [ASK] DOUBLE, PRIMARY KEY([INSTRUMENT_ID], [TIMESTAMP]))");
				st->step();

				m_db->set_schema_version(current_schema_version);

				t.commit();
			}
			catch (...)
			{
				t.rollback();
				throw;
			}

			LOG_INFO << "Oanda data storage DB schema has been created successfully!";
		}

		void data_storage::verify_db_schema()
		{
		}

		__int64 data_storage::get_instrument_row_id(const std::wstring& instrument_id)
		{
			auto insert_instrument_st = m_db->create_statement(L"INSERT INTO INSTRUMENTS (NAME) VALUES (?1)");
			auto select_instrument_id_st = m_db->create_statement(L"SELECT ID FROM INSTRUMENTS WHERE INSTRUMENTS.NAME = ?1");
			select_instrument_id_st->bind_value(instrument_id, 1);
			__int64 instrument_row_id = -1;
			if (select_instrument_id_st->step())
			{
				instrument_row_id = select_instrument_id_st->get_value<__int64>(0);
			}
			else
			{
				insert_instrument_st->bind_value(instrument_id, 1);
				insert_instrument_st->step();
				instrument_row_id = insert_instrument_st->last_insert_row_id();
			}

			return instrument_row_id;
		}

		std::vector<data_t::ptr> data_storage::get_data(const std::wstring& instrument_id, unsigned long granularity, tbp::time_t* start_datetime, tbp::time_t* end_datetime) const
		{
			if (nullptr == start_datetime || nullptr == end_datetime)
			{
				throw std::invalid_argument("start_datetime or end_datetime argument is null!");
			}

			auto st = m_db->create_statement(LR"(
				SELECT TIMESTAMP, BID_CANDLESTICK, ASK_CANDLESTICK, VOLUME 
					FROM INSTRUMENT_DATA 
					WHERE INSTRUMENT_ID IN (SELECT ID FROM INSTRUMENTS WHERE INSTRUMENTS.NAME = ?1) AND INSTRUMENT_DATA.TIMESTAMP >= ?2 AND INSTRUMENT_DATA.TIMESTAMP <= ?3 AND INSTRUMENT_DATA.GRANULARITY = ?4 ORDER BY TIMESTAMP ASC )");

			st->bind_value(instrument_id, 1);
			st->bind_value(start_datetime->time_since_epoch().count(), 2);
			st->bind_value(end_datetime->time_since_epoch().count(), 3);
			st->bind_value(static_cast<int>(granularity), 4);

			auto candels_data_st = m_db->create_statement(LR"(
				SELECT O_PRICE, H_PRICE, L_PRICE, C_PRICE 
					FROM CANDLES 
					WHERE ID = ?1 )");

			std::vector<data_t::ptr> result;
			while (st->step())
			{
				tbp::data_t record;
				record[values::instrument_data::c_timestamp] = tbp::time_t(tbp::time_t::duration(st->get_value<__int64>(0)));
				record[values::instrument_data::c_volume] = st->get_value<__int64>(3);

				candels_data_st->reset();
				candels_data_st->bind_value(st->get_value<__int64>(1), 1);
				if (candels_data_st->step())
				{
					record.emplace(values::instrument_data::c_bid_candlestick, read_candelstick_data(candels_data_st));
				}

				candels_data_st->reset();
				candels_data_st->bind_value(st->get_value<__int64>(2), 1);
				if (candels_data_st->step())
				{
					record.emplace(values::instrument_data::c_ask_candlestick, read_candelstick_data(candels_data_st));
				}

				result.emplace_back(std::make_shared<tbp::data_t>(std::move(record)));
			}

			if (result.size() <= 1)
			{
				*end_datetime = *start_datetime;

				return result;
			}

			{
				auto timestamp_it = (*result.begin())->find(tbp::oanda::values::instrument_data::c_timestamp);
				*start_datetime = tbp::get<tbp::time_t>(timestamp_it->second);
			}

			{
				auto timestamp_it = (*result.rbegin())->find(tbp::oanda::values::instrument_data::c_timestamp);
				*end_datetime = tbp::get<tbp::time_t>(timestamp_it->second);
			}

			return result;
		}

		std::vector<data_t::ptr> data_storage::get_instant_data(const std::wstring& instrument_id, tbp::time_t* start_datetime, tbp::time_t* end_datetime) const
		{
			if (nullptr == start_datetime || nullptr == end_datetime)
			{
				throw std::invalid_argument("start_datetime or end_datetime argument is null!");
			}

			auto st = m_db->create_statement(LR"(
				SELECT TIMESTAMP, BID, ASK
					FROM INSTANT_INSTRUMENT_DATA 
					WHERE INSTRUMENT_ID IN (SELECT ID FROM INSTRUMENTS WHERE INSTRUMENTS.NAME = ?1) AND INSTANT_INSTRUMENT_DATA.TIMESTAMP >= ?2 AND INSTANT_INSTRUMENT_DATA.TIMESTAMP <= ?3 ORDER BY TIMESTAMP ASC )");

			st->bind_value(instrument_id, 1);
			st->bind_value(start_datetime->time_since_epoch().count(), 2);
			st->bind_value(end_datetime->time_since_epoch().count(), 3);

			std::vector<data_t::ptr> result;
			while (st->step())
			{
				tbp::data_t record;
				record.emplace(values::instrument_data::c_timestamp, tbp::time_t(tbp::time_t::duration(st->get_value<__int64>(0))));
				record.emplace(values::instant_data::c_bid_price, st->get_value<double>(1));
				record.emplace(values::instant_data::c_ask_price, st->get_value<double>(2));

				result.emplace_back(std::make_shared<tbp::data_t>(std::move(record)));
			}

			if (result.size() <= 1)
			{
				*end_datetime = *start_datetime;

				return result;
			}

			{
				auto timestamp_it = (*result.begin())->find(tbp::oanda::values::instrument_data::c_timestamp);
				*start_datetime = tbp::get<tbp::time_t>(timestamp_it->second);
			}

			{
				auto timestamp_it = (*result.rbegin())->find(tbp::oanda::values::instrument_data::c_timestamp);
				*end_datetime = tbp::get<tbp::time_t>(timestamp_it->second);
			}

			return result;
		}

		void data_storage::save_data(const std::wstring& instrument_id, unsigned long granularity, const std::vector<data_t::ptr>& data)
		{
			sqlite::transaction t(m_db);

			try
			{
				const __int64 instrument_row_id = get_instrument_row_id(instrument_id);
				auto insert_candle_data_st = m_db->create_statement(L"INSERT INTO CANDLES(O_PRICE, H_PRICE, L_PRICE, C_PRICE) VALUES (?1, ?2, ?3, ?4)");
				auto insert_instrument_data_st = m_db->create_statement(L"INSERT OR REPLACE INTO INSTRUMENT_DATA(INSTRUMENT_ID, TIMESTAMP, GRANULARITY, BID_CANDLESTICK, ASK_CANDLESTICK, VOLUME) VALUES (?1, ?2, ?3, ?4, ?5, ?6)");
				for (const auto& instrument_data : data)
				{
					insert_instrument_data_st->reset();
					insert_instrument_data_st->bind_value(instrument_row_id, 1);

					insert_candle_data_st->reset();

					// TIMESTAMP
					{
						auto it = instrument_data->find(values::instrument_data::c_timestamp);
						if (instrument_data->end() == it)
						{
							throw std::runtime_error("TIMESTAMP value isn't provided by instrument data!");
						}

						insert_instrument_data_st->bind_value(boost::get<tbp::time_t>(it->second).time_since_epoch().count(), 2);
					}

					// GRANULARITY
					insert_instrument_data_st->bind_value(static_cast<int>(granularity), 3);

					std::wstring candlestick_values[] =
					{
						values::candlestick_data::c_open_price,
						values::candlestick_data::c_high_price,
						values::candlestick_data::c_low_price,
						values::candlestick_data::c_close_price,
					};

					auto save_candlestick_data = [&](const std::wstring& candlestick_name, int st_index)
					{
						auto it = instrument_data->find(candlestick_name);
						if (instrument_data->end() == it)
						{
							throw std::runtime_error("Candlestick value isn't provided by instrument data!");
						}

						const auto& candelstick_data = boost::get<tbp::data_t>(it->second);

						insert_candle_data_st->reset();

						int index = 1;
						for (const auto& value_name : candlestick_values)
						{
							auto it = candelstick_data.find(value_name);
							if (candelstick_data.end() == it)
							{
								throw std::runtime_error("Candlestick data incomplete!");
							}

							insert_candle_data_st->bind_value(boost::get<double>(it->second), index++);
						}

						insert_candle_data_st->step();

						insert_instrument_data_st->bind_value(insert_candle_data_st->last_insert_row_id(), st_index);
					};

					// BID_CANDLESTICK
					save_candlestick_data(values::instrument_data::c_bid_candlestick, 4);

					// ASK_CANDLESTICK
					save_candlestick_data(values::instrument_data::c_ask_candlestick, 5);

					// VOLUME
					{
						auto it = instrument_data->find(values::instrument_data::c_volume);
						if (instrument_data->end() == it)
						{
							throw std::runtime_error("VOLUME value isn't provided by instrument data!");
						}

						insert_instrument_data_st->bind_value(boost::get<__int64>(it->second), 6);
					}

					insert_instrument_data_st->step();
				}

				t.commit();
			}
			catch (...)
			{
				t.rollback();
				throw;
			}
		}

		void data_storage::save_instant_data(const std::wstring& instrument_id, const std::vector<data_t::ptr>& data)
		{
			sqlite::transaction t(m_db);

			try
			{
				const __int64 instrument_row_id = get_instrument_row_id(instrument_id);
				auto insert_instrument_data_st = m_db->create_statement(L"INSERT OR REPLACE INTO INSTANT_INSTRUMENT_DATA(INSTRUMENT_ID, TIMESTAMP, BID, ASK) VALUES (?1, ?2, ?3, ?4)");
				for (const auto& instrument_data : data)
				{
					insert_instrument_data_st->reset();
					insert_instrument_data_st->bind_value(instrument_row_id, 1);

					// TIMESTAMP
					{
						auto it = instrument_data->find(values::instrument_data::c_timestamp);
						if (instrument_data->end() == it)
						{
							throw std::runtime_error("TIMESTAMP value isn't provided by instant instrument data!");
						}

						insert_instrument_data_st->bind_value(tbp::get<tbp::time_t>(it->second).time_since_epoch().count(), 2);
					}

					// BID
					{
						auto it = instrument_data->find(values::instant_data::c_bid_price);
						if (instrument_data->end() == it)
						{
							throw std::runtime_error("BID value isn't provided by instant instrument data!");
						}

						insert_instrument_data_st->bind_value(tbp::get<double>(it->second), 3);
					}

					// ASK
					{
						auto it = instrument_data->find(values::instant_data::c_ask_price);
						if (instrument_data->end() == it)
						{
							throw std::runtime_error("ASK value isn't provided by instant instrument data!");
						}

						insert_instrument_data_st->bind_value(tbp::get<double>(it->second), 4);
					}

					insert_instrument_data_st->step();
				}

				t.commit();
			}
			catch (...)
			{
				t.rollback();
				throw;
			}
		}

		data_storage::data_storage(const sqlite::connection::ptr& db)
			: m_db(db)
		{
			create_db_schema();
		}
	}
}