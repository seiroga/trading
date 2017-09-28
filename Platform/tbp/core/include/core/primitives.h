#pragma once

#include <boost/variant.hpp>
#include <vector>
#include <map>
#include <chrono>

namespace tbp
{
	struct data_t;

	using byte_t = unsigned char;
	using time_t = std::chrono::system_clock::time_point;
	using binary_t = std::vector<byte_t>;
	using value_t = boost::variant<__int64, std::wstring, time_t, bool, double, binary_t, data_t>;

	struct data_t : public std::map<std::wstring, value_t>
	{
		using ptr = std::shared_ptr<data_t>;

	public:
		using std::map<std::wstring, value_t>::map;
	};

	using boost::get;
}