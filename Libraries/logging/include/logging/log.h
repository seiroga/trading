#pragma once

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <string>

namespace logging
{
	enum class level
	{
		debug,
		warning,
		error,
		info,
	};

	void init(level l, const std::wstring& path, bool replicate_on_cout);
}

#define LOG_DBG BOOST_LOG_TRIVIAL(debug)
#define LOG_WARN BOOST_LOG_TRIVIAL(warning)
#define LOG_ERR BOOST_LOG_TRIVIAL(error)
#define LOG_INFO BOOST_LOG_TRIVIAL(info)