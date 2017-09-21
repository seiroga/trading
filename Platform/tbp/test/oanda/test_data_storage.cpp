#include <boost/test/unit_test.hpp>

#include <oanda/data_storage.h>

#include <test_helpers/base_fixture.h>

#include <algorithm>
#include <functional>
#include <random>

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

				instrument_data[tbp::oanda::values::instrument_data::c_timestamp] = std::chrono::system_clock::now();
				instrument_data[tbp::oanda::values::instrument_data::c_volume] = __int64(rand());

				auto ask_price = generate_double();
				instrument_data[tbp::oanda::values::instrument_data::c_ask_candlestick] = generate_candle(ask_price);
				instrument_data[tbp::oanda::values::instrument_data::c_bid_candlestick] = generate_candle(ask_price - generate_delta());

				return std::make_shared<tbp::data_t>(std::move(instrument_data));
			});
		}
	};
}

BOOST_FIXTURE_TEST_CASE(save_data, common_fixture)
{
	// INIT (generate data)
	auto instrument_data = generate_data(100);


	tbp::oanda::data_storage ds();

	//// ASSERT
	//BOOST_ASSERT(data_arrived);
	//BOOST_ASSERT(ds->values.size() > 0);
	//for (auto& val : ds->values)
	//{
	//	BOOST_ASSERT(val == conn->value);
	//}
}