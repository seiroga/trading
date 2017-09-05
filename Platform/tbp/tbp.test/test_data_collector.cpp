#include "stdafx.h"
#include "CppUnitTest.h"

#include <data_collector.h>

#include <algorithm>
#include <functional>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace tbptest
{
	namespace 
	{
		struct mock_data_storage : public tbp::data_storage
		{
			std::vector<tbp::data_t::ptr> values;

			virtual std::vector<tbp::data_t::ptr> get_instrument_data(const std::wstring& instrument_id, tbp::time_t start_datetime, tbp::time_t end_datetime) override
			{
				return values;
			}

			virtual void save_instrument_data(const std::wstring& instrument_id, const std::vector<tbp::data_t::ptr>& data) override
			{
				for (const auto& d : data)
				{
					values.push_back(d);
				}
			}
		};

		struct mock_connector : public tbp::connector
		{
			tbp::data_t::ptr value;

			virtual std::vector<std::wstring> get_available_instruments() const override
			{
				return { L"" };
			}

			virtual tbp::data_t::ptr get_instrument_data(const std::wstring& instrument_id) override
			{
				return value;
			}

			virtual tbp::order::ptr create_order(const tbp::data_t& params) override
			{
				return nullptr;
			}
		};
	}

	TEST_CLASS(data_collector_start_stop)
	{
	public:
		
		TEST_METHOD(Test)
		{
			// INIT
			auto ds = std::make_shared<mock_data_storage>();
			auto conn = std::make_shared<mock_connector>();
			tbp::data_t mock_data({ { L"some_key", tbp::value_t(false) } });
			conn->value = std::make_shared<tbp::data_t>(mock_data);
			auto dc = std::make_unique<tbp::data_collector>(L"instrument_1", conn, ds);

			// ACT
			Sleep(1000);
			dc.reset();

			// ASSERT
			Assert::AreNotEqual(size_t(0), ds->values.size());
			for (auto& val : ds->values)
			{
				Assert::IsTrue(val == conn->value);
			}
		}

	};
}