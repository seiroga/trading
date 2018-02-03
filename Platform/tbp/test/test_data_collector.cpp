#include <boost/test/unit_test.hpp>

#include <mock/mock_connector.h>

#include <core/data_collector.h>

#include <test_helpers/base_fixture.h>

#include <boost/numeric/conversion/cast.hpp>

#include <algorithm>
#include <functional>
#include <fstream>

namespace 
{
	struct mock_data_storage : public tbp::data_storage
	{
		std::vector<tbp::data_t::ptr> values;
		std::vector<tbp::data_t::ptr> instant_values;
		win::event on_new_instant_data;
		win::event on_new_data;
		tbp::time_t start;
		tbp::time_t end;

		virtual std::vector<tbp::data_t::ptr> get_data(const std::wstring& instrument_id, unsigned long granularity, tbp::time_t* start_datetime, tbp::time_t* end_datetime) const override
		{
			*start_datetime = start;
			*end_datetime = end;

			return values;
		}

		virtual std::vector<tbp::data_t::ptr> get_instant_data(const std::wstring& instrument_id, tbp::time_t* start_datetime, tbp::time_t* end_datetime) const override
		{
			*start_datetime = start;
			*end_datetime = end;

			return instant_values;
		}

		virtual void save_data(const std::wstring& instrument_id, unsigned long granularity, const std::vector<tbp::data_t::ptr>& data) override
		{
			for (const auto& d : data)
			{
				values.push_back(d);
			}

			on_new_data.set();
		}

		virtual void save_instant_data(const std::wstring& instrument_id, const std::vector<tbp::data_t::ptr>& data) override
		{
			for (const auto& d : data)
			{
				instant_values.push_back(d);
			}

			on_new_instant_data.set();
		}

	public:
		mock_data_storage()
			: on_new_instant_data(true, false)
			, on_new_data(true, false)
		{
		}
	};

	struct common_fixture : test_helpers::base_fixture
	{
		tbp::settings::ptr settings;

	public:
		static auto align_to_granularity(const tbp::time_t& time, const std::chrono::seconds& granularity)
		{
			return (time.time_since_epoch() / granularity) * granularity;
		}

		bool is_aligned_to(const tbp::time_t& time, const std::chrono::seconds& granularity)
		{
			return time.time_since_epoch() == std::chrono::duration_cast<tbp::time_t::duration>(align_to_granularity(time, granularity));
		}

	public:
		common_fixture()
			: base_fixture(L"data_collector")
			, settings(tbp::settings::load_from_json(std::ifstream(get_file_path(L"app_settings.json"))))
		{
		}
	};
}

BOOST_FIXTURE_TEST_CASE(data_collector_collect_data, common_fixture)
{
	// SB: lets synchronize our requests with data granularity
	auto curr_time = tbp::time_t::clock::now();
	const auto time_granularity_secs = std::chrono::seconds(60);
	auto aligned_prev_time = (curr_time.time_since_epoch() / time_granularity_secs) * time_granularity_secs;
	auto diff = curr_time.time_since_epoch() - aligned_prev_time;
	std::chrono::seconds diff_in_secs = std::chrono::duration_cast<std::chrono::seconds>(diff);

	// INIT
	auto ds = std::make_shared<mock_data_storage>();
	auto conn = std::make_shared<mock_connector>();
	tbp::data_t mock_data({ { L"some_key", tbp::value_t(false) } });
	conn->value = std::make_shared<tbp::data_t>(mock_data);
	auto dc = std::make_unique<tbp::data_collector>(L"instrument_1", settings, conn, ds);
	dc->start();

	bool data_arrived = ds->on_new_instant_data.wait(2000);

	// ASSERT
	BOOST_ASSERT(data_arrived);
	BOOST_ASSERT(!ds->instant_values.empty());
	BOOST_ASSERT(!conn->instant_data_request_log.empty());

	// ACT
	auto actual_start = std::chrono::system_clock::now();
	auto actual_end = actual_start;
	dc->get_instant_data(L"instrument_1", &actual_start, &actual_end);
	dc.reset();

	// ASSERT
	BOOST_ASSERT(!ds->instant_values.empty());
	for (auto& val : ds->instant_values)
	{
		BOOST_ASSERT(val == conn->value);
	}
}

BOOST_FIXTURE_TEST_CASE(data_collector_create_delete, common_fixture)
{
	// INIT
	auto ds = std::make_shared<mock_data_storage>();
	auto conn = std::make_shared<mock_connector>();
	tbp::data_t mock_data({ { L"some_key", tbp::value_t(false) } });
	conn->value = std::make_shared<tbp::data_t>(mock_data);
	auto dc = std::make_unique<tbp::data_collector>(L"instrument_1", settings, conn, ds);
	dc->start();

	// ACT (no deadlock)
	dc.reset();
}

BOOST_FIXTURE_TEST_CASE(data_collector_request_data_from_connector_if_not_present_in_datastorage, common_fixture)
{
	// INIT
	auto ds = std::make_shared<mock_data_storage>();
	auto conn = std::make_shared<mock_connector>();
	tbp::data_t mock_data({ { L"some_key", tbp::value_t(false) } });
	conn->value = std::make_shared<tbp::data_t>(mock_data);
	auto dc = std::make_unique<tbp::data_collector>(L"instrument_1", settings, conn, ds);
	const auto delta = std::chrono::system_clock::duration(10000);
	dc->start();

	// ACT
	ds->start = std::chrono::system_clock::now();
	ds->end = ds->start + delta;
	ds->on_new_data.reset();
	const auto requested_start = ds->start - delta;
	const auto requested_end = ds->end + delta;
	auto actual_start = requested_start;
	auto actual_end = requested_end;
	dc->get_data(L"instrument_1", 5, &actual_start, &actual_end);

	// ASSERT
	BOOST_ASSERT(actual_start == requested_start);
	BOOST_ASSERT(actual_end == requested_end);
	BOOST_ASSERT(ds->on_new_data.wait(0));
	BOOST_ASSERT(conn->data_request_log.size() == 1);
	BOOST_ASSERT(conn->data_request_log[0].instrument_id == L"instrument_1");
	BOOST_ASSERT(conn->data_request_log[0].start == requested_start);
	BOOST_ASSERT(conn->data_request_log[0].end == requested_end);
}

BOOST_FIXTURE_TEST_CASE(data_collector_get_instant_data, common_fixture)
{
	// INIT
	auto ds = std::make_shared<mock_data_storage>();
	auto conn = std::make_shared<mock_connector>();
	tbp::data_t mock_data({ { L"some_key", tbp::value_t(false) } });
	conn->value = std::make_shared<tbp::data_t>(mock_data);
	auto dc = std::make_unique<tbp::data_collector>(L"instrument_1", settings, conn, ds);
	const auto delta = std::chrono::system_clock::duration(10000);
	dc->start();

	// ACT
	::Sleep(1000);
	ds->start = std::chrono::system_clock::now();
	ds->end = ds->start + delta;
	ds->on_new_instant_data.reset();
	const auto requested_start = ds->start - delta;
	const auto requested_end = ds->end + delta;
	auto actual_start = requested_start;
	auto actual_end = requested_end;
	auto instant_data = dc->get_instant_data(L"instrument_1", &actual_start, &actual_end);
	dc.reset();

	// ASSERT
	BOOST_ASSERT(actual_start == requested_start);
	BOOST_ASSERT(actual_end == requested_end);
	BOOST_ASSERT(ds->on_new_instant_data.wait(0));
	BOOST_ASSERT(ds->instant_values.size() == instant_data.size());
	BOOST_ASSERT(!conn->instant_data_request_log.empty());
	BOOST_ASSERT(*conn->instant_data_request_log.begin() == L"instrument_1");
	BOOST_ASSERT(*conn->instant_data_request_log.rbegin() == L"instrument_1");
}

BOOST_FIXTURE_TEST_CASE(data_collector_on_instant_data_signal, common_fixture)
{
	// INIT
	auto ds = std::make_shared<mock_data_storage>();
	auto conn = std::make_shared<mock_connector>();
	tbp::data_t mock_data({ { L"some_key", tbp::value_t(false) } });
	conn->value = std::make_shared<tbp::data_t>(mock_data);
	auto dc = std::make_unique<tbp::data_collector>(L"instrument_1", settings, conn, ds);

	// ACT
	bool signal_called = false;
	std::wstring signal_instrument_id;
	dc->on_instant_data.connect([&](const std::wstring& instrument_id, const std::vector<tbp::data_t::ptr>& data)
	{
		signal_called = true;
		signal_instrument_id = instrument_id;
	});

	dc->start();

	::Sleep(2000);
	dc.reset();

	// ASSERT
	BOOST_ASSERT(signal_called);
	BOOST_ASSERT(L"instrument_1" == signal_instrument_id);
}

BOOST_FIXTURE_TEST_CASE(data_collector_on_historical_data_signal, common_fixture)
{
	// INIT
	const auto granularity = std::chrono::seconds(tbp::get_value<int>(settings, L"DataGranulatiry"));
	const auto granularity_ms = std::chrono::duration_cast<std::chrono::milliseconds>(granularity);
	auto ds = std::make_shared<mock_data_storage>();
	auto conn = std::make_shared<mock_connector>();
	tbp::data_t mock_data({ { L"some_key", tbp::value_t(false) } });
	conn->value = std::make_shared<tbp::data_t>(mock_data);
	auto dc = std::make_unique<tbp::data_collector>(L"instrument_1", settings, conn, ds);

	// ACT
	size_t signal_called_count = 0;
	std::wstring signal_instrument_id;
	dc->on_historical_data.connect([&](const std::wstring& instrument_id, const std::vector<tbp::data_t::ptr>& data)
	{
		++signal_called_count;
		signal_instrument_id = instrument_id;
	});

	dc->start();

	const auto sleep_interval = boost::numeric_cast<unsigned long>(granularity_ms.count() * 3);
	::Sleep(sleep_interval);
	dc.reset();

	// ASSERT
	BOOST_ASSERT(signal_called_count != 0);
	BOOST_ASSERT(L"instrument_1" == signal_instrument_id);
	BOOST_ASSERT(conn->data_request_log.size() != 0);
	BOOST_ASSERT(ds->values.size() != 0);
	BOOST_ASSERT(ds->values.size() == conn->data_request_log.size());

	tbp::time_t prev_start = conn->data_request_log[0].start - granularity;
	for (const auto& info : conn->data_request_log)
	{
		BOOST_ASSERT(info.instrument_id == L"instrument_1");
		BOOST_ASSERT(is_aligned_to(info.start, granularity));
		BOOST_ASSERT(is_aligned_to(info.end, granularity));
		BOOST_ASSERT(granularity == std::chrono::duration_cast<std::chrono::seconds>(info.end - info.start));
		BOOST_ASSERT(granularity == std::chrono::duration_cast<std::chrono::seconds>(info.start - prev_start));

		prev_start = info.start;
	}
}