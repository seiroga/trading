#pragma once

#include <tbp/primitives.h>

#include <string>
#include <vector>
#include <memory>

namespace tbp
{
	struct order
	{
		using ptr = std::shared_ptr<order>;

	public:
		virtual void close() = 0;
		virtual void cancel() = 0;
	};

	class connector
	{
	public:
		using ptr = std::shared_ptr<connector>;

	public:
		virtual std::vector<std::wstring> get_available_instruments() const = 0;
		virtual data_t::ptr get_instrument_data(const std::wstring& instrument_id) = 0;
		virtual order::ptr create_order(const data_t& params) = 0;
	};
}