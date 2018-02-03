#pragma once

#include <core/primitives.h>

#include <common/constrains.h>

#include <boost/signals2.hpp>

#include <vector>
#include <memory>

namespace tbp
{
	struct data_provider : sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<data_provider>;

	public:
		boost::signals2::signal<void(const std::wstring& instrument_id, const std::vector<data_t::ptr>&)> on_instant_data;
		boost::signals2::signal<void(const std::wstring& instrument_id, const std::vector<data_t::ptr>&)> on_historical_data;

	public:
		virtual std::vector<data_t::ptr> get_data(const std::wstring& instrument_id, unsigned long granularity, time_t* start_datetime, time_t* end_datetime) const = 0;
		virtual std::vector<data_t::ptr> get_instant_data(const std::wstring& instrument_id, time_t* start_datetime, time_t* end_datetime) const = 0;
	};

	struct data_storage : data_provider
	{
	public:
		using ptr = std::shared_ptr<data_storage>;

	public:
		virtual void save_data(const std::wstring& instrument_id, unsigned long granularity, const std::vector<data_t::ptr>& data) = 0;
		virtual void save_instant_data(const std::wstring& instrument_id, const std::vector<data_t::ptr>& data) = 0;
	};
}