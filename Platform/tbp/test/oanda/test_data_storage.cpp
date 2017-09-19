#include <boost/test/unit_test.hpp>

#include <oanda/data_storage.h>

#include <algorithm>
#include <functional>
#include <random>

namespace
{
}

BOOST_AUTO_TEST_CASE(save_data)
{
	// INIT
	auto instrument_data = std::make_shared<tbp::data_t>();
	(*instrument_data)[tbp::oanda::values::instrument_data::c_timestamp] = std::chrono::system_clock::now();
	(*instrument_data)[tbp::oanda::values::instrument_data::c_volume] = __int64(std::random_device()());

	//// ASSERT
	//BOOST_ASSERT(data_arrived);
	//BOOST_ASSERT(ds->values.size() > 0);
	//for (auto& val : ds->values)
	//{
	//	BOOST_ASSERT(val == conn->value);
	//}
}