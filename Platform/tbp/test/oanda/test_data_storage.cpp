#include <boost/test/unit_test.hpp>

#include <oanda/data_storage.h>

#include <test_helpers/base_fixture.h>

#include <algorithm>
#include <functional>
#include <random>

#include <windows.h>

namespace
{
	struct common_fixture : test_helpers::temp_dir_fixture
	{
		std::random_device rand;

		double generate_double(long max_value = 1000)
		{
			return rand() % max_value;
		}

		auto generate_fractional_part()
		{
			return double((rand() % 1000) / 1000.0);
		};

		auto generate_delta(long max_value = 100)
		{
			return double(rand() % max_value) + generate_fractional_part();
		};

		tbp::data_t generate_candle(double value)
		{
			tbp::data_t candle_data;

			auto open_price = value + generate_fractional_part();
			auto low_price = open_price - generate_delta();

			candle_data[tbp::oanda::values::candlestick_data::c_open_price] = open_price;
			candle_data[tbp::oanda::values::candlestick_data::c_high_price] = open_price + generate_delta();
			candle_data[tbp::oanda::values::candlestick_data::c_low_price] = low_price;
			candle_data[tbp::oanda::values::candlestick_data::c_close_price] = low_price + generate_delta();

			return candle_data;
		}

		std::vector<tbp::data_t::ptr> generate_data(size_t count)
		{
			std::vector<tbp::data_t::ptr> result;
			std::generate_n(std::back_inserter(result), count, [&]()
			{
				tbp::data_t instrument_data;

				::Sleep(100);

				instrument_data[tbp::oanda::values::instrument_data::c_timestamp] = std::chrono::system_clock::now();
				instrument_data[tbp::oanda::values::instrument_data::c_volume] = __int64(rand());

				auto ask_price = generate_double();
				instrument_data[tbp::oanda::values::instrument_data::c_ask_candlestick] = generate_candle(ask_price);
				instrument_data[tbp::oanda::values::instrument_data::c_bid_candlestick] = generate_candle(ask_price - generate_delta());

				return std::make_shared<tbp::data_t>(std::move(instrument_data));
			});

			return result;
		}

		bool is_equal(const std::vector<tbp::data_t::ptr>& lhs, const std::vector<tbp::data_t::ptr>& rhs)
		{
			if (lhs.size() != rhs.size())
			{
				return false;
			}

			size_t index = 0;
			for (const auto& lhs_data_item : lhs)
			{
				if (*lhs_data_item != *rhs[index++])
				{
					return false;
				}
			}

			return true;
		}
	};
}

BOOST_FIXTURE_TEST_CASE(save_data, common_fixture)
{
	// INIT (generate data)
	const auto instrument_id = L"instrument1";
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	const auto start_time = std::chrono::system_clock::now();
	auto instrument_data = generate_data(100);
	const auto end_time = std::chrono::system_clock::now();

	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);
	tbp::oanda::data_storage ds(db);

	// ACT
	ds.save_instrument_data(instrument_id, instrument_data);
	auto data = ds.get_instrument_data(instrument_id, start_time, end_time);

	// ASSERT
	BOOST_ASSERT(is_equal(data, instrument_data));
}