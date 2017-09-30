#pragma once

#include <core/primitives.h>
#include <core/data_storage.h>

#include <common/constrains.h>

#include <string>
#include <vector>
#include <memory>

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
		using ptr = std::shared_ptr<order>;

	public:
		virtual void close() = 0;
		virtual void cancel() = 0;
	};

	class connector : data_provider
	{
	public:
		using ptr = std::shared_ptr<connector>;

	public:
		using data_provider::get_instrument_data;

		virtual std::vector<std::wstring> get_instruments() const = 0;
		virtual data_t::ptr get_instrument_data(const std::wstring& instrument_id) = 0;
		virtual order::ptr create_order(const data_t& params) = 0;
	};
}