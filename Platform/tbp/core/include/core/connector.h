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
	/////////////////////////////////////////////////////////////////////
	// authentication

	struct authentication : sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<authentication>;

	public:
		virtual void login() = 0;
		virtual std::wstring get_token() const = 0;
	};

	/////////////////////////////////////////////////////////////////////
	// order

	struct order : sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<order>;

		enum class state_t
		{
			pending,
			filled,
			canceled
		};

	public:
		virtual std::wstring id() const = 0;
		virtual std::wstring trade_id() const = 0;
		virtual state_t state() const = 0;
		virtual void cancel() = 0;
	};

	/////////////////////////////////////////////////////////////////////
	// trade

	struct trade : sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<trade>;

		enum class state_t
		{
			opened,
			closed
		};

	public:
		virtual std::wstring id() const = 0;
		virtual state_t state() const = 0;
		virtual double amount() const = 0;
		virtual double profit(bool unrealized) const = 0;
		virtual void close(double amount_to_close) = 0;
	};

	/////////////////////////////////////////////////////////////////////
	// http exception

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

	/////////////////////////////////////////////////////////////////////
	// connector

	struct connector : sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<connector>;

	public:
		virtual std::vector<std::wstring> get_instruments() const = 0;
		virtual double available_balance() const = 0;
		virtual double margin_rate() const = 0;
		virtual std::vector<data_t::ptr> get_data(const std::wstring& instrument_id, unsigned long granularity, time_t* start_datetime, time_t* end_datetime) const = 0;
		virtual data_t::ptr get_instant_data(const std::wstring& instrument_id) = 0;
		virtual order::ptr create_order(const data_t& params) = 0;
		virtual order::ptr find_order(const std::wstring& id) const = 0;
		virtual trade::ptr find_trade(const std::wstring& id) const = 0;
	};
}