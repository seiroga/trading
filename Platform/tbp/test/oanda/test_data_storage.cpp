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
		const unsigned long default_granularity = 5;

		template<typename T>
		T generate(long max_value = 1000)
		{
			return rand() % max_value;
		}

		auto generate_fractional_part()
		{
			return generate<double>(1000) / 1000.0;
		};

		auto generate_delta(long max_value = 100)
		{
			return generate<double>(max_value) + generate_fractional_part();
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

				::Sleep(10);

				instrument_data[tbp::oanda::values::instrument_data::c_timestamp] = std::chrono::system_clock::now();
				instrument_data[tbp::oanda::values::instrument_data::c_volume] = __int64(rand());

				auto ask_price = generate<double>();
				instrument_data[tbp::oanda::values::instrument_data::c_ask_candlestick] = generate_candle(ask_price);
				instrument_data[tbp::oanda::values::instrument_data::c_bid_candlestick] = generate_candle(ask_price - generate_delta());

				return std::make_shared<tbp::data_t>(std::move(instrument_data));
			});

			return result;
		}

		std::vector<tbp::data_t::ptr> generate_instant_data(size_t count)
		{
			std::vector<tbp::data_t::ptr> result;
			std::generate_n(std::back_inserter(result), count, [&]()
			{
				tbp::data_t instrument_data;

				::Sleep(10);

				instrument_data[tbp::oanda::values::instrument_data::c_timestamp] = std::chrono::system_clock::now();
				instrument_data[tbp::oanda::values::instant_data::c_bid_price] = generate<double>(10000) + generate_fractional_part();
				instrument_data[tbp::oanda::values::instant_data::c_ask_price] = generate<double>(10000) + generate_fractional_part();

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

		tbp::time_t get_timestamp(const tbp::data_t::ptr& data)
		{
			auto it = data->find(tbp::oanda::values::instrument_data::c_timestamp);
			if (data->end() == it)
			{
				throw std::runtime_error("Test data is corrupted!");
			}

			return tbp::get<tbp::time_t>(it->second);
		}
	};
}

BOOST_FIXTURE_TEST_CASE(save_data, common_fixture)
{
	// INIT (generate data)
	const auto instrument_id = L"instrument1";
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto start_time = std::chrono::system_clock::now();
	auto instrument_data = generate_data(100);
	auto end_time = std::chrono::system_clock::now();

	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);
	tbp::oanda::data_storage ds(db);

	// ACT
	ds.save_data(instrument_id, default_granularity, instrument_data);
	auto data = ds.get_data(instrument_id, default_granularity, &start_time, &end_time);

	// ASSERT
	BOOST_ASSERT(start_time == get_timestamp(*data.begin()));
	BOOST_ASSERT(end_time == get_timestamp(*data.rbegin()));
	BOOST_ASSERT(is_equal(data, instrument_data));
}

BOOST_FIXTURE_TEST_CASE(get_data, common_fixture)
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
	ds.save_data(instrument_id, default_granularity, instrument_data);
	auto start_time2 = get_timestamp(instrument_data[10]);
	auto end_time2 = get_timestamp(instrument_data[50]);
	auto data = ds.get_data(instrument_id, default_granularity, &start_time2, &end_time2);

	// ASSERT
	BOOST_ASSERT(start_time2 == get_timestamp(instrument_data[10]));
	BOOST_ASSERT(end_time2 == get_timestamp(instrument_data[50]));
	BOOST_ASSERT(is_equal(data, std::vector<tbp::data_t::ptr>(instrument_data.begin() + 10, instrument_data.begin() + 51)));
}

BOOST_FIXTURE_TEST_CASE(update_data, common_fixture)
{
	// INIT (generate data)
	const auto instrument_id = L"instrument1";
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto start_time = std::chrono::system_clock::now();
	auto instrument_data = generate_data(100);
	auto end_time = std::chrono::system_clock::now();

	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);
	tbp::oanda::data_storage ds(db);

	// ACT
	ds.save_data(instrument_id, default_granularity, instrument_data);
	auto data = ds.get_data(instrument_id, default_granularity, &start_time, &end_time);

	// ASSERT
	BOOST_ASSERT(is_equal(data, instrument_data));

	// INIT
	const auto index = generate<size_t>(99);
	const auto ask_price = generate<double>();
	(*instrument_data[index])[tbp::oanda::values::instrument_data::c_ask_candlestick] = generate_candle(ask_price);
	(*instrument_data[index])[tbp::oanda::values::instrument_data::c_bid_candlestick] = generate_candle(ask_price - generate_delta());

	// ACT
	ds.save_data(instrument_id, default_granularity, instrument_data);
	data = ds.get_data(instrument_id, default_granularity, &start_time, &end_time);

	// ASSERT
	BOOST_ASSERT(is_equal(data, instrument_data));
}

BOOST_FIXTURE_TEST_CASE(get_data_requested_range_bigger_than_available, common_fixture)
{
	// INIT (generate data)
	const auto instrument_id = L"instrument1";
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto start_time = std::chrono::system_clock::now();
	auto instrument_data = generate_data(100);
	auto end_time = std::chrono::system_clock::now();

	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);
	tbp::oanda::data_storage ds(db);

	// ACT
	ds.save_data(instrument_id, default_granularity, instrument_data);
	auto available_start = start_time - tbp::time_t::duration(10000);
	auto available_end = end_time + tbp::time_t::duration(10000);
	auto data = ds.get_data(instrument_id, default_granularity, &available_start, &available_end);

	// ASSERT
	BOOST_ASSERT(available_start == get_timestamp(*instrument_data.begin()));
	BOOST_ASSERT(available_end == get_timestamp(*instrument_data.rbegin()));
	BOOST_ASSERT(is_equal(data, instrument_data));
}

BOOST_FIXTURE_TEST_CASE(save_instant_data, common_fixture)
{
	// INIT (generate data)
	const auto instrument_id = L"instrument1";
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto start_time = std::chrono::system_clock::now();
	auto instrument_data = generate_instant_data(100);
	auto end_time = std::chrono::system_clock::now();

	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);
	tbp::oanda::data_storage ds(db);

	// ACT
	ds.save_instant_data(instrument_id, instrument_data);
	auto data = ds.get_instant_data(instrument_id, &start_time, &end_time);

	// ASSERT
	BOOST_ASSERT(start_time == get_timestamp(*data.begin()));
	BOOST_ASSERT(end_time == get_timestamp(*data.rbegin()));
	BOOST_ASSERT(is_equal(data, instrument_data));
}

BOOST_FIXTURE_TEST_CASE(get_instant_data, common_fixture)
{
	// INIT (generate data)
	const auto instrument_id = L"instrument1";
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	const auto start_time = std::chrono::system_clock::now();
	auto instrument_data = generate_instant_data(100);
	const auto end_time = std::chrono::system_clock::now();

	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);
	tbp::oanda::data_storage ds(db);

	// ACT
	ds.save_instant_data(instrument_id, instrument_data);
	auto start_time2 = get_timestamp(instrument_data[10]);
	auto end_time2 = get_timestamp(instrument_data[50]);
	auto data = ds.get_instant_data(instrument_id, &start_time2, &end_time2);

	// ASSERT
	BOOST_ASSERT(start_time2 == get_timestamp(instrument_data[10]));
	BOOST_ASSERT(end_time2 == get_timestamp(instrument_data[50]));
	BOOST_ASSERT(is_equal(data, std::vector<tbp::data_t::ptr>(instrument_data.begin() + 10, instrument_data.begin() + 51)));
}

BOOST_FIXTURE_TEST_CASE(data_with_different_garnurality_independent, common_fixture)
{
	// INIT (generate data)
	const auto instrument_id = L"instrument1";
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	const auto start_time = std::chrono::system_clock::now();
	auto instrument_data = generate_data(100);
	const auto end_time = std::chrono::system_clock::now();

	const auto granularity2 = 10;
	const auto start_time_g2 = std::chrono::system_clock::now();
	auto instrument_data_g2 = generate_data(100);
	const auto end_time_g2 = std::chrono::system_clock::now();

	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);
	tbp::oanda::data_storage ds(db);

	// ACT
	ds.save_data(instrument_id, default_granularity, instrument_data);
	auto start_time2 = get_timestamp(instrument_data[10]);
	auto end_time2 = get_timestamp(instrument_data[50]);
	auto data = ds.get_data(instrument_id, default_granularity, &start_time2, &end_time2);

	ds.save_data(instrument_id, granularity2, instrument_data_g2);
	auto start_time2_g2 = get_timestamp(instrument_data_g2[10]);
	auto end_time2_g2 = get_timestamp(instrument_data_g2[50]);
	auto data_g2 = ds.get_data(instrument_id, granularity2, &start_time2_g2, &end_time2_g2);

	// ASSERT
	BOOST_ASSERT(start_time2 == get_timestamp(instrument_data[10]));
	BOOST_ASSERT(end_time2 == get_timestamp(instrument_data[50]));
	BOOST_ASSERT(is_equal(data, std::vector<tbp::data_t::ptr>(instrument_data.begin() + 10, instrument_data.begin() + 51)));

	BOOST_ASSERT(start_time2_g2 == get_timestamp(instrument_data_g2[10]));
	BOOST_ASSERT(end_time2_g2 == get_timestamp(instrument_data_g2[50]));
	BOOST_ASSERT(is_equal(data_g2, std::vector<tbp::data_t::ptr>(instrument_data_g2.begin() + 10, instrument_data_g2.begin() + 51)));
}