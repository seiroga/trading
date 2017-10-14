#include <oanda/connector.h>
#include <oanda/data_storage.h>

#include <win/exception.h>

#include <common/string_cvt.h>

#include <cpprest/http_client.h>
#include <cpprest/producerconsumerstream.h>
#include <cpprest/json.h>

#include <regex>
#include <time.h>

#include <windows.h>

namespace tbp
{
	namespace oanda
	{
		/////////////////////////////////////////////////////////////////////////
		// authentication implemnetation

		void authentication::login()
		{
			// Do nothing. We work through predefined access token
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
					std::to_wstring(st.wMinute) + L":" + std::to_wstring(st.wSecond) + (0 != st.wMilliseconds ? L"." + std::to_wstring(st.wMilliseconds) : L"") + L"Z";
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
					st.wMilliseconds = _wtoi(std::wstring(*submatches_iterator).c_str());
				}

				// SB: convert to time_t
				const auto epoch_time_t = std::chrono::system_clock::to_time_t(time_t());
				const auto epoch_time = gmtime(&epoch_time_t);
				st.wYear = st.wYear - (epoch_time->tm_year + 1900) + 1601;
				FILETIME ft = { 0 };
				if (0 == SystemTimeToFileTime(&st, &ft))
				{
					throw std::runtime_error("SystemTimeToFileTime call failed!");
				}

				return tbp::time_t(tbp::time_t::duration(*reinterpret_cast<__int64*>(&ft)));
			}

			double to_double(const std::wstring& str)
			{
				return wcstod(str.c_str(), nullptr);
			}

			/////////////////////////////////////////////////////////////////////////
			// schema implemnetation

			struct schema
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

				std::wstring get_instant_prices_url(const std::wstring& account_id, const std::vector<std::wstring>& instruments) const
				{
					web::uri_builder uri_path(base_url);
					uri_path.append_path(api_version);
					uri_path.append_path(L"accounts");
					uri_path.append_path(account_id);
					uri_path.append_path(L"pricing");
					uri_path.append_query(L"instruments" + create_comma_sep_values(instruments), true);

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

			public:
				schema(const std::wstring& base_url, const std::wstring& api_version)
					: base_url(base_url)
					, api_version(api_version)
				{
				}
			};

			/////////////////////////////////////////////////////////////////////////
			// connector implemnetation

			class connector_impl : public tbp::connector
			{
				const std::wstring m_token;
				const schema m_schema;

			private:
				web::http::http_request create_request(web::http::method m) const
				{
					web::http::http_request request(m);
					auto& header = request.headers();
					header[L"Authorization"] = L"Bearer " + m_token;
					header[L"Content-Type"] = L"application/json";
					header[L"Accept-Datetime-Format"] = L"RFC3339";

					return request;
				}

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
				virtual std::vector<data_t::ptr> get_data(const std::wstring& instrument_id, time_t* start, time_t* end) const override
				{
					using namespace web::json;

					concurrency::streams::container_buffer<std::vector<tbp::byte_t>> buffer;

					// Create http_client to send the request.
					web::http::client::http_client client(m_schema.get_historical_prices_url(instrument_id, L"S5", *start, *end));
					auto request = create_request(web::http::methods::GET);

					// Handle response headers arriving.
					auto requestTask = client.request(request).then([&](web::http::http_response response)
					{
						// Write response body into the file.
						return response.body().read_to_end(buffer);
					});

					// Wait for all the outstanding I/O to complete
					requestTask.wait();

					std::wstring str_responce = sb::to_str(std::string(buffer.collection().begin(), buffer.collection().end()));
					auto json_responce = web::json::value::parse(str_responce);

					std::vector<data_t::ptr> result;
					{
						const auto& candles_arr = json_responce.as_object().at(L"candles").as_array();
						for (const auto& candle_data : candles_arr)
						{
							auto candle_info = std::make_shared<tbp::data_t>();
							const auto& candle_obj = candle_data.as_object();
							candle_info->insert({ values::instrument_data::c_timestamp, parse_time(candle_obj.at(L"time")) });
							candle_info->insert({ values::instrument_data::c_volume, static_cast<__int64>(candle_obj.at(L"volume").as_integer()) });
							candle_info->insert({ values::instrument_data::c_bid_candlestick, parse_candle_object(candle_obj.at(L"bid").as_object()) });
							candle_info->insert({ values::instrument_data::c_ask_candlestick, parse_candle_object(candle_obj.at(L"ask").as_object()) });

							result.push_back(candle_info);
						}
					}

					return result;
				}

			public:
				virtual std::vector<std::wstring> get_instruments() const override
				{
					return {};
				}

				virtual data_t::ptr get_instant_data(const std::wstring& instrument_id) override
				{
					return nullptr;
				}

				virtual order::ptr create_order(const data_t& params) override
				{
					return nullptr;
				}

			public:
				connector_impl(const authentication::ptr& auth)
					: m_token(auth->get_token())
					, m_schema(L"https://api-fxpractice.oanda.com", L"v3")
				{
				}

				~connector_impl()
				{
				}
			};
		}

		tbp::connector::ptr connector::create(const authentication::ptr& auth)
		{
			return std::make_shared<connector_impl>(auth);
		}
	}
}