#pragma once

#include <tbp/primitives.h>

#include <common/constrains.h>

#include <vector>
#include <memory>

namespace tbp
{
	struct data_provider : sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<data_provider>;

	public:
		virtual std::vector<data_t::ptr> get_instrument_data(const std::wstring& instrument_id, time_t start_datetime, time_t end_datetime) = 0;
	};

	struct data_storage : data_provider
	{
	public:
		using ptr = std::shared_ptr<data_storage>;

	public:
		virtual void save_instrument_data(const std::wstring& instrument_id, const std::vector<data_t::ptr>& data) = 0;
	};
}