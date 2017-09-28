#include <boost/test/unit_test.hpp>

#include <core/data_collector.h>

#include <algorithm>
#include <functional>

namespace 
{
	struct mock_data_storage : public tbp::data_storage
	{
		std::vector<tbp::data_t::ptr> values;
		win::event on_new_data;
		tbp::time_t start;
		tbp::time_t end;

		virtual std::vector<tbp::data_t::ptr> get_instrument_data(const std::wstring& instrument_id, tbp::time_t* start_datetime, tbp::time_t* end_datetime) const override
		{
			*start_datetime = start;
			*end_datetime = end;

			return values;
		}

		virtual void save_instrument_data(const std::wstring& instrument_id, const std::vector<tbp::data_t::ptr>& data) override
		{
			for (const auto& d : data)
			{
				values.push_back(d);
			}

			on_new_data.set();
		}

	public:
		mock_data_storage()
			: on_new_data(true, false)
		{
		}
	};

	struct mock_connector : public tbp::connector
	{
		struct request_info
		{
			const std::wstring instrument_id;
			const tbp::time_t start;
			const tbp::time_t end;
		};

		tbp::data_t::ptr value;
		mutable std::vector<request_info> data_request_log;

		virtual std::vector<std::wstring> get_instruments() const override
		{
			return { L"" };
		}

		virtual tbp::data_t::ptr get_instrument_data(const std::wstring& instrument_id) override
		{
			return value;
		}

		virtual std::vector<tbp::data_t::ptr> get_instrument_data(const std::wstring& instrument_id, tbp::time_t* start_datetime, tbp::time_t* end_datetime) const override
		{
			data_request_log.push_back({ instrument_id, *start_datetime, *end_datetime });
			return { value };
		}

		virtual tbp::order::ptr create_order(const tbp::data_t& params) override
		{
			return nullptr;
		}
	};
}

BOOST_AUTO_TEST_CASE(data_collector_process_data)
{
	// INIT
	auto ds = std::make_shared<mock_data_storage>();
	auto conn = std::make_shared<mock_connector>();
	tbp::data_t mock_data({ { L"some_key", tbp::value_t(false) } });
	conn->value = std::make_shared<tbp::data_t>(mock_data);
	auto dc = std::make_unique<tbp::data_collector>(L"instrument_1", conn, ds);

	// ACT
	const bool data_arrived = ds->on_new_data.wait(2000);
	dc.reset();

	// ASSERT
	BOOST_ASSERT(data_arrived);
	BOOST_ASSERT(ds->values.size() > 0);
	for (auto& val : ds->values)
	{
		BOOST_ASSERT(val == conn->value);
	}
}

BOOST_AUTO_TEST_CASE(data_collector_create_delete)
{
	// INIT
	auto ds = std::make_shared<mock_data_storage>();
	auto conn = std::make_shared<mock_connector>();
	tbp::data_t mock_data({ { L"some_key", tbp::value_t(false) } });
	conn->value = std::make_shared<tbp::data_t>(mock_data);
	auto dc = std::make_unique<tbp::data_collector>(L"instrument_1", conn, ds);

	// ACT (no deadlock)
	dc.reset();
}

BOOST_AUTO_TEST_CASE(data_collector_request_data_from_connector_if_not_present_in_datastorage)
{
	// INIT
	auto ds = std::make_shared<mock_data_storage>();
	auto conn = std::make_shared<mock_connector>();
	tbp::data_t mock_data({ { L"some_key", tbp::value_t(false) } });
	conn->value = std::make_shared<tbp::data_t>(mock_data);
	auto dc = std::make_unique<tbp::data_collector>(L"instrument_1", conn, ds);
	const auto delta = std::chrono::system_clock::duration(10000);

	// ACT
	ds->start = std::chrono::system_clock::now();
	ds->end = ds->start + delta;
	ds->on_new_data.reset();
	const auto requested_start = ds->start - delta;
	const auto requested_end = ds->end + delta;
	auto actual_start = requested_start;
	auto actual_end = requested_end;
	dc->get_instrument_data(L"instrument_1", &actual_start, &actual_end);

	// ASSERT
	BOOST_ASSERT(actual_start == requested_start);
	BOOST_ASSERT(actual_end == requested_end);
	BOOST_ASSERT(ds->on_new_data.wait(0));
	BOOST_ASSERT(conn->data_request_log.size() == 1);
	BOOST_ASSERT(conn->data_request_log[0].instrument_id == L"instrument_1");
	BOOST_ASSERT(conn->data_request_log[0].start == requested_start);
	BOOST_ASSERT(conn->data_request_log[0].end == requested_end);
}