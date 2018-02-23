#include <oanda/trader.h>
#include <oanda/connector.h>
#include <oanda/data_storage.h>

#include <logging/log.h>
#include <sqlite/sqlite.h>

#include <win/fs.h>
#include <win/com/utils.h>

#include <boost/numeric/conversion/cast.hpp>

#include <functional>

namespace tbp
{
	namespace oanda
	{
		namespace
		{
			const int current_schema_version = 1;

			std::wstring get_db_path(const std::wstring& working_dir)
			{
				using win::fs::operator/;

				return working_dir / L"DB" / L"oanda" / L"trading.db";
			}

			std::wstring generate_id()
			{
				return win::com::guid_to_str(win::com::generate_guid());
			}

			tbp::candle_info get_candle_info(const tbp::data_t& candle_data)
			{
				tbp::candle_info result;
				result.open = tbp::get<double>(candle_data.at(values::candlestick_data::c_open_price));
				result.high = tbp::get<double>(candle_data.at(values::candlestick_data::c_high_price));
				result.low = tbp::get<double>(candle_data.at(values::candlestick_data::c_low_price));
				result.close = tbp::get<double>(candle_data.at(values::candlestick_data::c_close_price));

				return result;
			}
		}

		///////////////////////////////////////////////////////////////////////////////////////////
		// trading_db implementation

		class trader::trading_db
		{
		public:
			struct object_id
			{
				std::wstring remote_id;
				std::wstring internal_id;
			};

		private:
			sqlite::connection::ptr m_db;

		private:
			static sqlite::connection::ptr open_db(const std::wstring& db_path)
			{
				const auto exists = win::fs::exists(db_path);
				LOG_INFO << (exists ? L"Open existing trading DB..." : L"Creating trading DB...");

				win::fs::create_path(db_path.substr(0, db_path.find_last_of(L"\\")));
				auto db = sqlite::connection::create(db_path);

				LOG_INFO << L"Successfully connected to trading DB!";

				return db;
			}

			void create_schema()
			{
				const auto version = m_db->schema_version();
				switch (version)
				{
				case 0:
					break;

				case current_schema_version:
					return;

				default:
					throw std::runtime_error("Unknown trading DB schema version!");
				}

				sqlite::transaction t(m_db);

				try
				{
					LOG_INFO << "Oanda data storage DB schema is empty. Creating DB chema!";

					auto st = m_db->create_statement(L"CREATE TABLE [IDS]([ID] INTEGER PRIMARY KEY NOT NULL, [LOCAL_ID] TEXT, [REMOTE_ID] TEXT)");
					st->step();

					st = m_db->create_statement(L"CREATE TABLE [ORDERS]([ID] REFERENCES IDS(ID) ON DELETE CASCADE, [CREATED] INTEGER, [PROCESSED] INTEGER, [STATE] INTEGER, [LINKED_TRADE] REFERENCES IDS(ID))");
					st->step();

					st = m_db->create_statement(L"CREATE TABLE [TRADES]([ID] REFERENCES IDS(ID) ON DELETE CASCADE, [OPENED] INTEGER, [STATE] INTEGER, [LINKED_ORDER] REFERENCES IDS(ID))");
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

		public:
			std::vector<object_id> get_pending_trades()
			{
				auto st = m_db->create_statement(LR"(
					SELECT REMOTE_ID, LOCAL_ID
					FROM IDS 
					WHERE ID IN (SELECT ID FROM TRADES WHERE TRADES.STATE = ?1)
					)");

				st->bind_value(static_cast<int>(trade::state_t::opened), 1);

				std::vector<object_id> result;
				while (st->step())
				{
					result.push_back({ st->get_value<std::wstring>(0), st->get_value<std::wstring>(1) });
				}

				return result;
			}

			std::vector<object_id> get_pending_orders()
			{
				auto st = m_db->create_statement(LR"(
					SELECT REMOTE_ID, LOCAL_ID
					FROM IDS 
					WHERE ID IN (SELECT ID FROM ORDERS WHERE ORDERS.STATE = ?1)
					)");

				st->bind_value(static_cast<int>(order::state_t::pending), 1);

				std::vector<object_id> result;
				while (st->step())
				{
					result.push_back({ st->get_value<std::wstring>(0), st->get_value<std::wstring>(1) });
				}

				return result;
			}

			void set_order_state(const std::wstring& internal_id, order::state_t state)
			{
				auto st = m_db->create_statement(LR"(
					UPDATE ORDERS
					SET STATE = ?1 WHERE ID IN (SELECT ID FROM IDS WHERE IDS.LOCAL_ID = ?2)
					)");

				st->bind_value(static_cast<int>(state), 1);
				st->bind_value(internal_id, 2);

				if (!st->step())
				{
					LOG_DBG << L"Update order state to " << int(state) << L". Order internal ID: " << internal_id;
				}
			}

			void set_trade_state(const std::wstring& internal_id, trade::state_t state)
			{
				auto st = m_db->create_statement(LR"(
					UPDATE TRADES
					SET STATE = ?1 WHERE ID IN (SELECT ID FROM IDS WHERE IDS.LOCAL_ID = ?2)
					)");

				st->bind_value(static_cast<int>(state), 1);
				st->bind_value(internal_id, 2);

				if (!st->step())
				{
					LOG_DBG << L"Update trade state to " << int(state) << L". Trade internal ID: " << internal_id;
				}
			}

			std::wstring get_remote_id(const std::wstring& internal_id) const
			{
				auto st = m_db->create_statement(LR"(
					SELECT REMOTE_ID
					FROM IDS 
					WHERE IDS.LOCAL_ID = ?1
					)");

				st->bind_value(internal_id, 1);

				if (!st->step())
				{
					throw std::runtime_error("Could not find remote ID by internal ID! InternalID: " + sb::to_str(internal_id));
				}
				{
					return st->get_value<std::wstring>(0);
				}
			}

			void register_order(const std::wstring& internal_id, const tbp::order::ptr& order, const tbp::trade::ptr& linked_trade)
			{
				sqlite::transaction t(m_db);

				try
				{
					auto insert_into_ids = [this](const auto& internal_id, const auto& remote_id)
					{
						auto st = m_db->create_statement(LR"(
							INSERT OR FAIL INTO IDS(LOCAL_ID, REMOTE_ID)
							VALUES (?1, ?2)
							)");

						st->bind_value(internal_id, 1);
						st->bind_value(remote_id, 2);

						st->step();

						return st->last_insert_row_id();
					};

					// SB: insert order into IDS
					const __int64 order_ids_row_id = insert_into_ids(internal_id, order->id());

					// SB: insert into TRADES
					__int64 trade_ids_row_id = 0;
					if (nullptr != linked_trade)
					{
						// SB: insert trade into IDS
						trade_ids_row_id = insert_into_ids(linked_trade->id(), linked_trade->id());

						auto st = m_db->create_statement(LR"(
							INSERT OR FAIL INTO TRADES(ID, OPENED, STATE, LINKED_ORDER)
							VALUES (?1, ?2, ?3, ?4)
							)");

						const auto open_time = std::chrono::system_clock::now().time_since_epoch().count();
						st->bind_value(trade_ids_row_id, 1);
						st->bind_value(open_time, 2);
						st->bind_value(int(linked_trade->state()), 3);
						st->bind_value(order_ids_row_id, 4);

						st->step();
					}

					// SB: insert into ORDERS
					{
						auto st = m_db->create_statement(LR"(
							INSERT OR FAIL INTO ORDERS(ID, CREATED, PROCESSED, STATE, LINKED_TRADE)
							VALUES (?1, ?2, ?3, ?4, ?5)
							)");

						st->bind_value(order_ids_row_id, 1);

						const auto creation_time = std::chrono::system_clock::now().time_since_epoch().count();
						st->bind_value(creation_time, 2);

						const auto state = order->state();
						if (tbp::order::state_t::filled == state || tbp::order::state_t::canceled == state)
						{
							st->bind_value(creation_time, 3);
						}
						else
						{
							// SB: order is in pending state
							st->bind_value(sqlite::vnull, 3);
						}

						st->bind_value(int(state), 4);
						st->bind_value(nullptr != linked_trade ? sqlite::value_t(trade_ids_row_id) : sqlite::vnull, 5);

						st->step();
					}

					t.commit();
				}
				catch (...)
				{
					t.rollback();

					throw;
				}
			}

		public:
			trading_db(const std::wstring& db_path)
				: m_db(open_db(db_path))
			{
				create_schema();
			}

			~trading_db()
			{
			}
		};

		/////////////////////////////////////////////////////////////////////////////////////////////
		// trader implementation

		std::wstring trader::open_trade(const std::wstring& instrument_id, double amount)
		{
			std::wstring internal_id;

			// SB: create MarketOrder request
			auto order = m_connector->create_order(create_market_order_params(instrument_id, amount));
			switch (order->state())
			{
			case tbp::order::state_t::filled:
				{
					internal_id = generate_id();
					auto trade = m_connector->find_trade(order->trade_id());
					if (nullptr == trade)
					{
						LOG_ERR << L"Order \"" << order->id() << L"\" was filled. But linked trade \"" << order->trade_id() << "\" wasn't found by connector!";
					}

					m_db->register_order(internal_id, order, trade);
				}
				break;

			case tbp::order::state_t::pending:
				{
					internal_id = generate_id();
					m_db->register_order(internal_id, order, nullptr);
				}
				break;

			case tbp::order::state_t::canceled:
				{
					internal_id = generate_id();
					m_db->register_order(internal_id, order, nullptr);

					throw trade_canceled("Order was created successfully, but was canceled by broker!");
				}
				break;
			}

			return internal_id;
		}

		void trader::close_trade(const std::wstring& internal_id, double amount)
		{
			const auto remote_id = m_db->get_remote_id(internal_id);
			auto order = m_connector->find_order(remote_id);
			if (nullptr == order)
			{
				LOG_ERR << L"Can't find remote order by ID! While trying to close trade! Order remote ID: " + remote_id;
				
				return;
			}

			auto state = order->state();
			switch (state)
			{
			case order::state_t::pending:
				{
					order->cancel();
					state = order->state();
					m_db->set_order_state(internal_id, state);

					LOG_DBG << L"Canceling pending order, no trade was opened. Order internal ID: " + internal_id;

					break;
				}

			case order::state_t::filled:
				{
					m_db->set_order_state(internal_id, state);
					auto trade_id = order->trade_id();
					auto trade = m_connector->find_trade(trade_id);
					if (nullptr == trade)
					{
						LOG_ERR << L"Can't find and close related trade for specified order. Order internal ID: " + internal_id + L". Trade remote ID: " + trade_id;
						
						break;
					}

					if (trade::state_t::opened == trade->state())
					{
						trade->close(amount);
						m_db->set_trade_state(trade_id, trade->state());

						LOG_DBG << (L"Trade closed. Trade remote ID: " + trade_id + L". Realized profit: " + std::to_wstring(trade->profit(false)));
					}
					else
					{
						LOG_DBG << (L"Trade already closed. Trade remote ID: " + trade_id);
					}

					break;
				}

			case order::state_t::canceled:
				{
					LOG_DBG << L"Order already canceled. Order remote ID: " + remote_id;

					break;
				}
			}
		}

		void trader::update_objects_states()
		{
			// SB: update all pending orders
			auto pending_orders = m_db->get_pending_orders();
			for (const auto& order_id : pending_orders)
			{
				auto order = m_connector->find_order(order_id.remote_id);
				if (nullptr != order)
				{
					m_db->set_order_state(order_id.internal_id, order->state());
				}
				else
				{
					LOG_WARN << L"Unable update order state. Order wasn't found by connector. Order remote ID: " << order_id.remote_id;
				}
			}

			// SB: update all pending trades
			auto pending_trades = m_db->get_pending_trades();
			for (const auto& trade_id : pending_trades)
			{
				auto trade = m_connector->find_trade(trade_id.remote_id);
				if (nullptr != trade)
				{
					m_db->set_trade_state(trade_id.internal_id, trade->state());
				}
				else
				{
					LOG_WARN << L"Unable update trade state. Trade wasn't found by connector. Trade remote ID: " << trade_id.remote_id;
				}
			}
		}

		void trader::close_pending_trades()
		{
			LOG_DBG << L"Closing all opened trades...";

			// SB: cancel all pending orders
			auto pending_orders = m_db->get_pending_orders();
			for (const auto& order_id : pending_orders)
			{
				auto order = m_connector->find_order(order_id.remote_id);
				if (nullptr != order)
				{
					auto state = order->state();
					if (order::state_t::pending == state)
					{
						order->cancel();
						state = order->state();

						LOG_DBG << L"Order has been canceld. Remote ID: " + order_id.remote_id;
					}

					m_db->set_order_state(order_id.internal_id, state);
				}
				else
				{
					LOG_WARN << L"Unable to close order!. Order wasn't found by connector. Order remote ID: " << order_id.remote_id;
				}
			}

			// SB: close all pending trades
			auto pending_trades = m_db->get_pending_trades();
			for (const auto& trade_id : pending_trades)
			{
				auto trade = m_connector->find_trade(trade_id.remote_id);
				if (nullptr != trade)
				{
					trade->close(0.0);
					m_db->set_trade_state(trade->id(), trade->state());

					LOG_DBG << (L"Trade has been closed. Remote ID: " + trade_id.remote_id + L". Realized profit: " + std::to_wstring(trade->profit(false)));
				}
				else
				{
					LOG_WARN << L"Unable to close trade!. Trade wasn't found by connector. Trade remote ID: " << trade_id.remote_id;
				}
			}

			LOG_DBG << L"All pending trades has beed closed!";
		}

		std::vector<candlestick_data> trader::get_candles_from_data(const std::vector<data_t::ptr>& candles_data) const
		{
			std::vector<candlestick_data> result;
			for (const auto& candle_data : candles_data)
			{
				candlestick_data candle;
				
				// VOLUME
				candle.volume = boost::numeric_cast<int>(tbp::get<__int64>(candle_data->at(values::instrument_data::c_volume)));

				// TIMESTAMP
				candle.timestamp = tbp::get<time_t>(candle_data->at(values::instrument_data::c_timestamp));

				// BID
				candle.bid = get_candle_info(tbp::get<data_t>(candle_data->at(values::instrument_data::c_bid_candlestick)));

				// ASK
				candle.ask = get_candle_info(tbp::get<data_t>(candle_data->at(values::instrument_data::c_ask_candlestick)));

				// COMPLETE
				// SB: when data is got from connector it contains "complete" attribute
				// if from DB, data doesn't contain "complete" attribute
				// by default complete attribute is set to true
				auto complete_it = candle_data->find(values::instrument_data::c_complete);
				if (candle_data->end() != complete_it)
				{
					candle.complete = tbp::get<bool>(complete_it->second);
				}

				result.emplace_back(std::move(candle));
			}

			return result;
		}

		trader::trader(const tbp::connector::ptr& c, const std::wstring& working_dir)
			: m_connector(c)
			, m_db(std::make_unique<trading_db>(get_db_path(working_dir)))
		{
			if (nullptr == m_connector)
			{
				throw std::invalid_argument("Connector object is empty!");
			}

			update_objects_states();

			LOG_DBG << L"Trader has been created successfully!";
		}

		trader::~trader()
		{
			LOG_ERR << L"Begin trader shutdown...";

			try
			{
				close_pending_trades();
			}
			catch (...)
			{
				LOG_ERR << L"Exception was thrown during closing all opened trades!";
			}

			LOG_DBG << L"Trader has been destroyed!";
		}
	}
}