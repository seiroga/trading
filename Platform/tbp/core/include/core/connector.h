#pragma once

#include <core/primitives.h>
#include <core/data_storage.h>

#include <common/constrains.h>
#include <common/string_cvt.h>

#include <string>
#include <vector>
#include <memory>
#include <exception>

namespace tbp
{
	struct authentication : sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<authentication>;

	public:
		virtual void login() = 0;
		virtual std::wstring get_token() const = 0;
	};

	struct order : sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<order>;

	public:
		virtual void close() = 0;
		virtual void cancel() = 0;
	};

	struct http_exception : public std::exception
	{
		unsigned long code;

	public:
		http_exception(unsigned long code, const std::wstring& err_info)
			: http_exception(code, sb::to_str(err_info))
		{
		}

		http_exception(unsigned long code, const std::string& err_info)
			: http_exception(code, err_info.c_str())
		{
		}

		http_exception(unsigned long code, const char* err_info)
			: code(code)
			, std::exception(err_info)
		{
		}
	};

	struct connector : sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<connector>;

	public:
		virtual std::vector<std::wstring> get_instruments() const = 0;
		virtual std::vector<data_t::ptr> get_data(const std::wstring& instrument_id, time_t* start_datetime, time_t* end_datetime) const = 0;
		virtual data_t::ptr get_instant_data(const std::wstring& instrument_id) = 0;
		virtual order::ptr create_order(const data_t& params) = 0;
	};
}