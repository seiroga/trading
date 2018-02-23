#include <oanda/connector.h>
#include <oanda/data_storage.h>

#include <win/exception.h>

#include <common/string_cvt.h>

#include <logging/log.h>

#include <cpprest/http_client.h>
#include <cpprest/producerconsumerstream.h>
#include <cpprest/json.h>

#include <regex>
#include <time.h>

#include <windows.h>
#include <winhttp.h>

namespace tbp
{
	namespace oanda
	{
		/////////////////////////////////////////////////////////////////////////
		// authentication implemnetation

		void authentication::login()
		{
			// Do nothing. We work through predefined access token

			LOG_INFO << L"Login is not required. Work with access toked directly!";
		}

		std::wstring authentication::get_token() const
		{
			return m_access_token;
		}

		authentication::authentication(const std::wstring& access_token)
			: m_access_token(access_token)
		{
		}

		authentication::~authentication()
		{
		}

		namespace
		{
			std::wstring create_comma_sep_values(const std::vector<std::wstring>& values)
			{
				std::wstring result;
				for (const auto& val : values)
				{
					if (result.empty())
					{
						result = val;
						continue;
					}

					result += L"," + val;
				}

				return result;
			}

			std::wstring to_str(time_t time)
			{
				const auto epoch_time_t = std::chrono::system_clock::to_time_t(time_t());
				auto epoch_time = gmtime(&epoch_time_t);
				auto raw_time = time.time_since_epoch().count();
				SYSTEMTIME st = { 0 };
				if (0 == ::FileTimeToSystemTime(reinterpret_cast<const FILETIME*>(&raw_time), &st))
				{
					throw win::exception(L"FileTimeToSystemTime call failed!");
				}

				// according to https://tools.ietf.org/rfc/rfc3339.txt
				return std::to_wstring(st.wYear - 1601 + epoch_time->tm_year + 1900) + L"-" + std::to_wstring(st.wMonth) + L"-" + std::to_wstring(st.wDay) + L"T" + std::to_wstring(st.wHour) + L":" + 
					std::to_wstring(st.wMinute) + L":" + std::to_wstring(st.wSecond) + (0 != st.wMilliseconds ? L"." + std::to_wstring(st.wMilliseconds) : L".000000000") + L"Z";
			}

			tbp::time_t to_time(const std::wstring& str)
			{
				std::wregex rex(L"(\\d{4})-(\\d{1,2})-(\\d{1,2})T(\\d{1,2}):(\\d{1,2}):(\\d{1,2})\\.*(\\d{0,10})Z");

				auto get_iterator = [](std::wsregex_token_iterator& it)
				{
					if (std::wsregex_token_iterator() == it)
					{
						throw std::runtime_error("Invlid datetime string format!");
					}

					return it;
				};

				std::wsregex_token_iterator submatches_iterator(str.begin(), str.end(), rex, {1, 2, 3, 4, 5, 6, 7});
				SYSTEMTIME st = { 0 };
				st.wYear = _wtoi(std::wstring(*get_iterator(submatches_iterator)).c_str());
				st.wMonth = _wtoi(std::wstring(*get_iterator(++submatches_iterator)).c_str());
				st.wDay = _wtoi(std::wstring(*get_iterator(++submatches_iterator)).c_str());
				st.wHour = _wtoi(std::wstring(*get_iterator(++submatches_iterator)).c_str());
				st.wMinute = _wtoi(std::wstring(*get_iterator(++submatches_iterator)).c_str());
				st.wSecond = _wtoi(std::wstring(*get_iterator(++submatches_iterator)).c_str());
				
				// SB: read milliseconds if exists
				if (std::wsregex_token_iterator() != ++submatches_iterator)
				{
					//SB: according to https://msdn.microsoft.com/en-us/library/windows/desktop/ms724950(v=vs.85).aspx
					auto ms_str = std::wstring(*submatches_iterator);
					auto ms = _wtoi(ms_str.c_str());
					if (ms_str.size() > 3)
					{
						double ms_d = ms / pow(10, ms_str.size() - 3);
						ms_d = round(ms_d);
						ms = (int)ms_d;
					}

					st.wMilliseconds = ms;
				}

				// SB: convert to time_t
				const auto epoch_time_t = std::chrono::system_clock::to_time_t(time_t());
				const auto epoch_time = gmtime(&epoch_time_t);
				st.wYear = st.wYear - (epoch_time->tm_year + 1900) + 1601;
				FILETIME ft = { 0 };
				if (0 == SystemTimeToFileTime(&st, &ft))
				{
					throw win::exception(L"SystemTimeToFileTime call failed!");
				}

				return tbp::time_t(tbp::time_t::duration(*reinterpret_cast<__int64*>(&ft)));
			}

			double to_double(const std::wstring& str)
			{
				return wcstod(str.c_str(), nullptr);
			}

			web::json::value to_json(const tbp::data_t& data)
			{
				struct value_visitor : boost::static_visitor<web::json::value>
				{
					web::json::value result;

					auto operator()(__int64 val)
					{
						return web::json::value(std::to_wstring(val));
					}
					
					auto operator()(const std::wstring& val)
					{
						return web::json::value(val);
					}

					auto operator()(const time_t& val)
					{
						return web::json::value(to_str(val));
					}
					
					auto operator()(bool val)
					{
						return web::json::value(val);
					}
					
					auto operator()(double val)
					{
						return web::json::value(std::to_wstring(val));
					}
					
					web::json::value operator()(const binary_t& val)
					{
						throw std::runtime_error("Unsupported json value!");
					}
					
					auto operator()(const data_t& val)
					{
						for (const auto& it : val)
						{
							result[it.first] = it.second.apply_visitor(value_visitor());
						}

						return result;
					}

				public:
					value_visitor()
						: result(web::json::value::parse(L"{}"))
					{
					}
				};

				return value_visitor()(data);
			}

			// SB: grnularity expressed in secs
			std::wstring granularity_to_str(unsigned long granularity)
			{
				switch (granularity)
				{
				case 5:
					return L"S5";

				case 10:
					return L"S10";

				case 15:
					return L"S15";

				case 30:
					return L"S30";

				case 60:
					return L"M1";

				case 120:
					return L"M2";

				case 240:
					return L"M4";

				case 300:
					return L"M5";

				case 600:
					return L"M10";

				case 900:
					return L"M15";

				case 1800:
					return L"M30";

				case 3600:
					return L"H1";

				case 7200:
					return L"H2";

				case 10800:
					return L"H3";

				case 14400:
					return L"H4";

				case 21600:
					return L"H6";

				case 28800:
					return L"H8";

				case 43200:
					return L"H12";

				case 86400:
					return L"D";

				case 604800:
					return L"W";

				case 2592000:
					return L"M";
				}

				throw std::runtime_error("Invalid granularity specified!");
			}

			/////////////////////////////////////////////////////////////////////////
			// schema implemnetation

			struct service_schema
			{
				const std::wstring base_url;
				const std::wstring api_version;

			public:
				std::wstring get_instruments_url(const std::wstring& account_id) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"accounts");
					uri_path.append_path(account_id);
					uri_path.append_path(L"instruments");

					return uri_path.to_string();
				}

				std::wstring get_account_info_url(const std::wstring& account_id) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"accounts");
					uri_path.append_path(account_id);

					return uri_path.to_string();
				}

				std::wstring get_instant_prices_url(const std::wstring& account_id, const std::vector<std::wstring>& instruments) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"accounts");
					uri_path.append_path(account_id);
					uri_path.append_path(L"pricing");
					uri_path.append_query(L"instruments", create_comma_sep_values(instruments));

					return uri_path.to_string();
				}

				std::wstring get_historical_prices_url(const std::wstring& instrument, const std::wstring& granularity, time_t utc_start, time_t utc_end) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"instruments");
					uri_path.append_path(instrument);
					uri_path.append_path(L"candles");
					uri_path.append_query(L"price", L"BA");
					uri_path.append_query(L"granularity", granularity);
					uri_path.append_query(L"from", to_str(utc_start));
					uri_path.append_query(L"to", to_str(utc_end));

					return uri_path.to_string();
				}

				std::wstring get_historical_prices_url(const std::wstring& instrument, const std::wstring& granularity, time_t utc_start) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"instruments");
					uri_path.append_path(instrument);
					uri_path.append_path(L"candles");
					uri_path.append_query(L"price", L"BA");
					uri_path.append_query(L"granularity", granularity);
					uri_path.append_query(L"from", to_str(utc_start));

					return uri_path.to_string();
				}

				std::wstring create_order_url(const std::wstring& account_id) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"accounts");
					uri_path.append_path(account_id);
					uri_path.append_path(L"orders");

					return uri_path.to_string();
				}

				std::wstring get_order_info_url(const std::wstring& account_id, const std::wstring& order_id) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"accounts");
					uri_path.append_path(account_id);
					uri_path.append_path(L"orders");
					uri_path.append_path(order_id);

					return uri_path.to_string();
				}

				std::wstring cancel_order_url(const std::wstring& account_id, const std::wstring& order_id) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"accounts");
					uri_path.append_path(account_id);
					uri_path.append_path(L"orders");
					uri_path.append_path(order_id);
					uri_path.append_path(L"cancel");

					return uri_path.to_string();
				}

				std::wstring get_transaction_info_url(const std::wstring& account_id, const std::wstring& transaction_id) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"accounts");
					uri_path.append_path(account_id);
					uri_path.append_path(L"transactions");
					uri_path.append_path(transaction_id);

					return uri_path.to_string();
				}

				std::wstring get_trade_info_url(const std::wstring& account_id, const std::wstring& trade_id) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"accounts");
					uri_path.append_path(account_id);
					uri_path.append_path(L"trades");
					uri_path.append_path(trade_id);

					return uri_path.to_string();
				}

				std::wstring close_trade_url(const std::wstring& account_id, const std::wstring& trade_id) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"accounts");
					uri_path.append_path(account_id);
					uri_path.append_path(L"trades");
					uri_path.append_path(trade_id);
					uri_path.append_path(L"close");

					return uri_path.to_string();
				}

			public:
				service_schema(const std::wstring& base_url, const std::wstring& api_version)
					: base_url(base_url)
					, api_version(api_version)
				{
				}
			};

			/////////////////////////////////////////////////////////////////////
			// service_client

			struct service_client : public sb::dynamic
			{
				using ptr = std::shared_ptr<service_client>;

			public:
				const std::wstring token;
				const std::wstring account_id;
				const std::shared_ptr<service_schema> schema;

			public:
				web::http::http_request create_request(web::http::method m, const web::json::value& body)
				{
					web::http::http_request request(m);
					auto& header = request.headers();
					header[L"Authorization"] = L"Bearer " + token;
					header[L"Content-Type"] = L"application/json";
					header[L"Accept-Datetime-Format"] = L"RFC3339";

					if (!body.is_null())
					{
						request.set_body(body);
					}

					return request;
				}

				web::json::value execute_request(web::http::method method, const std::wstring& url, const web::json::value& body = web::json::value())
				{
					try
					{
						// Create http_client to send the request.
						web::http::client::http_client client(url);
						auto request = create_request(method, body);

						web::json::value json_response;
						std::exception_ptr exception;

						// Handle response headers arriving.
						auto requestTask = client.request(request).then([&](web::http::http_response response)
						{
							if (HTTP_STATUS_BAD_REQUEST <= response.status_code())
							{
								exception = std::make_exception_ptr(http_exception(response.status_code(), response.reason_phrase()));
							}

							return response.extract_json();

						}).then([&](const web::json::value& response)
						{
							json_response = response;
						});

						// Wait for all the outstanding I/O to complete
						requestTask.wait();

						if (nullptr != exception)
						{
							const auto error_code = json_response.has_field(L"errorCode") ? json_response.at(L"errorCode").as_string() : L"Not specified";
							const auto error_message = json_response.has_field(L"errorMessage") ? json_response.at(L"errorMessage").as_string() : L"Not specified";

							LOG_DBG << L"Request failed with next server info. " << L"Error code: " << error_code << L". Error message: " << error_message;

							std::rethrow_exception(exception);
						}

						return json_response;
					}
					catch (const web::http::http_exception& ex)
					{
						throw http_exception(ex.error_code().value(), ex.what());
					}
				}

			public:
				service_client(const std::wstring& token, const std::wstring& account_id, const std::shared_ptr<service_schema>& schema)
					: token(token)
					, account_id(account_id)
					, schema(schema)
				{
				}
			};

			/////////////////////////////////////////////////////////////////////
			// order_impl

			class order_impl : public tbp::order
			{
				service_client::ptr m_service;
				const std::wstring m_id;
				const std::wstring m_trade_id;

			public:
				virtual std::wstring id() const override
				{
					return m_id;
				}

				virtual std::wstring trade_id() const override
				{
					return m_trade_id;
				}

				virtual state_t state() const override
				{
					auto url = m_service->schema->get_order_info_url(m_service->account_id, m_id);
					auto response = m_service->execute_request(web::http::methods::GET, url);
					const auto order_state = response.at(L"order").at(L"state").as_string();
					if (L"PENDING" == order_state)
					{
						return state_t::pending;
					}
					else if(L"FILLED" == order_state)
					{
						return state_t::filled;
					}
					else if (L"CANCELLED" == order_state)
					{
						return state_t::canceled;
					}

					// SB: strange state...
					throw std::runtime_error(sb::to_str(L"Unsupported order state! State: " + order_state));
				}

				virtual void cancel() override
				{
					switch (state())
					{
					case state_t::pending:
						{
							const auto url = m_service->schema->cancel_order_url(m_service->account_id, m_id);
							m_service->execute_request(web::http::methods::PUT, url);

							LOG_DBG << L"Order has been canceled. Order ID: " << m_id;
						}
						break;

					case state_t::filled:
						{
							LOG_DBG << L"Unable to cancel order. Order has been already filled. Order ID: " << m_id;
						}
						break;

					case state_t::canceled:
						{
							LOG_DBG << L"Order has been already canceled. Order ID: " << m_id;
						}
						break;
					}
				}

			public:
				order_impl(const std::wstring& id, const std::wstring& trade_id, const service_client::ptr& service)
					: m_service(service)
					, m_id(id)
					, m_trade_id(trade_id)
				{
				}
			};

			/////////////////////////////////////////////////////////////////////////
			// trade_impl

			class trade_impl : public tbp::trade
			{
				service_client::ptr m_service;
				const std::wstring m_id;

			private:
				auto get_trade_info() const
				{
					const auto url = m_service->schema->get_trade_info_url(m_service->account_id, m_id);
					auto response = m_service->execute_request(web::http::methods::GET, url);
					return response.at(L"trade");
				}

			public:
				virtual std::wstring id() const override
				{
					return m_id;
				}

				virtual state_t state() const override
				{
					auto trade_info = get_trade_info();
					const auto& state_val = trade_info.at(L"state").as_string();
					if (L"OPEN" == state_val)
					{
						return state_t::opened;
					}
					else if (L"CLOSED" == state_val)
					{
						return state_t::closed;
					}
					else if (L"CLOSE_WHEN_TRADEABLE" == state_val)
					{
						// SB: returns "opened" since it hasn't been closed yet
						return state_t::opened;
					}

					throw std::runtime_error("Unsupported trade state was returned!");
				}

				virtual double amount() const override
				{
					auto trade_info = get_trade_info();

					return to_double(trade_info.at(L"currentUnits").as_string());
				}

				virtual double profit(bool unrealized) const override
				{
					auto trade_info = get_trade_info();

					return to_double(trade_info.at(unrealized ? L"unrealizedPL" : L"realizedPL").as_string());
				}

				virtual void close(double amount_to_close) override
				{
					// SB: for now ignore amount_to_close, always close trade fully
					const auto url = m_service->schema->close_trade_url(m_service->account_id, m_id);
					m_service->execute_request(web::http::methods::PUT, url);
				}

			public:
				trade_impl(const std::wstring& id, const service_client::ptr& service)
					: m_id(id)
					, m_service(service)
				{
				}
			};

			/////////////////////////////////////////////////////////////////////////
			// connector_impl implemnetation

			class connector_impl : public tbp::connector
			{
				const service_client::ptr m_service;

			private:
				web::json::value execute_request(web::http::method method, const std::wstring& url, const web::json::value& body = web::json::value()) const
				{
					return m_service->execute_request(method, url, body);
				}

			private:
				static tbp::data_t parse_candle_object(const web::json::object& candle_data)
				{
					tbp::data_t result;

					result.insert({ values::candlestick_data::c_close_price, tbp::value_t(to_double(candle_data.at(L"c").as_string())) });
					result.insert({ values::candlestick_data::c_high_price, tbp::value_t(to_double(candle_data.at(L"h").as_string())) });
					result.insert({ values::candlestick_data::c_low_price, tbp::value_t(to_double(candle_data.at(L"l").as_string())) });
					result.insert({ values::candlestick_data::c_open_price, tbp::value_t(to_double(candle_data.at(L"o").as_string())) });

					return result;
				}

				static tbp::time_t parse_time(const web::json::value& time_val)
				{
					const auto& time_str = time_val.as_string();
					return to_time(time_str);
				}

			public:
				virtual std::vector<data_t::ptr> get_data(const std::wstring& instrument_id, unsigned long granularity, time_t* start, time_t* end) const override
				{
					auto url = nullptr != end ? m_service->schema->get_historical_prices_url(instrument_id, granularity_to_str(granularity), *start, *end) :
						m_service->schema->get_historical_prices_url(instrument_id, granularity_to_str(granularity), *start);
					auto json_responce = execute_request(web::http::methods::GET, url);

					std::vector<data_t::ptr> result;
					{
						const auto& candles_arr = json_responce.as_object().at(L"candles").as_array();
						for (const auto& candle_data : candles_arr)
						{
							tbp::data_t candle_info;
							const auto& candle_obj = candle_data.as_object();
							candle_info.insert({ values::instrument_data::c_timestamp, parse_time(candle_obj.at(L"time")) });
							candle_info.insert({ values::instrument_data::c_volume, static_cast<__int64>(candle_obj.at(L"volume").as_integer()) });
							candle_info.insert({ values::instrument_data::c_bid_candlestick, parse_candle_object(candle_obj.at(L"bid").as_object()) });
							candle_info.insert({ values::instrument_data::c_ask_candlestick, parse_candle_object(candle_obj.at(L"ask").as_object()) });
							candle_info.insert({ values::instrument_data::c_complete, candle_obj.at(L"complete").as_bool() });

							result.emplace_back(std::make_shared<tbp::data_t>(std::move(candle_info)));
						}
					}

					return result;
				}

			public:
				virtual std::vector<std::wstring> get_instruments() const override
				{
					auto json_responce = execute_request(web::http::methods::GET, m_service->schema->get_instruments_url(m_service->account_id));

					std::vector<std::wstring> result;
					auto instruments = json_responce.at(L"instruments").as_array();
					for (const auto& instrument_info : instruments)
					{
						result.push_back(instrument_info.at(L"name").as_string());
					}

					return result;
				}

				virtual double available_balance() const override
				{
					auto json_responce = execute_request(web::http::methods::GET, m_service->schema->get_account_info_url(m_service->account_id));

					std::vector<std::wstring> result;
					auto account_balance = json_responce.at(L"account").at(L"balance");

					return to_double(account_balance.as_string());
				}

				virtual double margin_rate() const override
				{
					auto json_responce = execute_request(web::http::methods::GET, m_service->schema->get_account_info_url(m_service->account_id));

					std::vector<std::wstring> result;
					auto account_balance = json_responce.at(L"account").at(L"marginRate");

					return to_double(account_balance.as_string());
				}

				virtual data_t::ptr get_instant_data(const std::wstring& instrument_id) override
				{
					auto json_responce = execute_request(web::http::methods::GET, m_service->schema->get_instant_prices_url(m_service->account_id, { instrument_id }));

					tbp::data_t result;
					auto prices = json_responce.at(L"prices").as_array();
					for (const auto& price_info : prices)
					{
						result.emplace(tbp::oanda::values::instrument_data::c_timestamp, to_time(price_info.at(L"time").as_string()));

						{
							// ask price
							// read first one from array
							auto ask_prices = price_info.at(L"asks").as_array();
							result.emplace(tbp::oanda::values::instant_data::c_ask_price, ask_prices.size() > 0 ? to_double(ask_prices.at(0).at(L"price").as_string()) : 0.0);
						}

						{
							// bid price
							// read first one from array
							auto bid_prices = price_info.at(L"bids").as_array();
							result.emplace(tbp::oanda::values::instant_data::c_bid_price, bid_prices.size() > 0 ? to_double(bid_prices.at(0).at(L"price").as_string()) : 0.0);
						}
					}

					return std::make_shared<tbp::data_t>(std::move(result));
				}

				virtual order::ptr create_order(const data_t& params) override
				{
					auto order_info = execute_request(web::http::methods::POST, m_service->schema->create_order_url(m_service->account_id), to_json(params));
					auto create_transaction = order_info.at(L"orderCreateTransaction");
					if (L"MARKET_ORDER" == create_transaction.at(L"type").as_string())
					{
						if (order_info.has_field(L"orderFillTransaction"))
						{
							auto order_fill_transaction = order_info.at(L"orderFillTransaction");
							const auto order_id = order_fill_transaction.at(L"orderID").as_string();
							std::wstring trade_id;
							if (order_fill_transaction.has_field(L"tradeOpened"))
							{
								trade_id = order_fill_transaction.at(L"tradeOpened").at(L"tradeID").as_string();
							}

							return std::make_shared<order_impl>(order_id, trade_id, m_service);
						}
						else if (order_info.has_field(L"orderCancelTransaction"))
						{
							auto order_cancel_transaction = order_info.at(L"orderCancelTransaction");
							const auto order_id = order_cancel_transaction.at(L"orderID").as_string();

							return std::make_shared<order_impl>(order_id, L"", m_service);
						}
					}

					throw std::runtime_error("Unsupported order type!");
				}

				virtual order::ptr find_order(const std::wstring& id) const override
				{
					auto url = m_service->schema->get_order_info_url(m_service->account_id, id);
					auto response = m_service->execute_request(web::http::methods::GET, url);
					const auto order_info = response.at(L"order");
					const auto order_type = order_info.at(L"type").as_string();
					if (L"MARKET" == order_type)
					{
						std::wstring linked_trade_id;
						if (L"FILLED" == order_info.at(L"state").as_string())
						{
							linked_trade_id = order_info.at(L"tradeOpenedID").as_string();
						}

						return std::make_shared<order_impl>(id, linked_trade_id, m_service);
					}

					throw std::runtime_error(sb::to_str(L"Unsupported order type! Order type: " + order_type));
				}

				virtual trade::ptr find_trade(const std::wstring& id) const override
				{
					// SB: just to check that trade exists
					const auto url = m_service->schema->get_trade_info_url(m_service->account_id, id);
					auto response = m_service->execute_request(web::http::methods::GET, url);

					return std::make_shared<trade_impl>(id, m_service);
				}

			public:
				connector_impl(const settings::ptr& settings, const authentication::ptr& auth)
					: m_service(std::make_shared<service_client>(auth->get_token(), get_value<std::wstring>(settings, L"account_id"), std::make_shared<service_schema>(get_value<std::wstring>(settings, L"url"), L"v3")))
				{
					LOG_DBG << L"OANDA Connector has been created successfully!";
				}

				~connector_impl()
				{
					LOG_DBG << L"Connector has been destroyed!";
				}
			};
		}

		///////////////////////////////////////////////////////////////////////////////////////
		// market order params

		data_t create_market_order_params(const std::wstring& instrument_id, double amount)
		{
			data_t params;
			params[L"units"] = amount;
			params[L"instrument"] = instrument_id;
			params[L"timeInForce"] = std::wstring(L"FOK");
			params[L"type"] = std::wstring(L"MARKET");
			params[L"positionFill"] = std::wstring(L"DEFAULT");

			data_t order_info;
			order_info[L"order"] = std::move(params);

			return order_info;
		}

		///////////////////////////////////////////////////////////////////////////////////////
		// connector

		tbp::connector::ptr connector::create(const settings::ptr& settings, const authentication::ptr& auth)
		{
			return std::make_shared<connector_impl>(settings, auth);
		}
	}
}