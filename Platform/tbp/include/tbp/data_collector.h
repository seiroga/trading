#pragma once

#include <tbp/primitives.h>

#include <vector>
#include <memory>

namespace tbp
{
	struct data_provider
	{
		using ptr = std::shared_ptr<data_provider>;

	public:
		virtual std::vector<data_t::ptr> get_instrument_data(const std::wstring& instrument_id, time_t start_datetime, time_t end_datetime) = 0;
	};

	struct data_collector
	{
	public:
		virtual data_provider::ptr create_data_provider() = 0;
	};
}