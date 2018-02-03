#pragma once

#include <core/data_storage.h>
#include <core/connector.h>
#include <core/settings.h>

#include<win/thread.h>

#include <thread>
#include <string>

namespace tbp
{
	class data_collector : public data_provider
	{
	public:
		using ptr = std::shared_ptr<data_collector>;

	private:
		const size_t m_cache_size;
		const unsigned long m_historcial_data_granularity;
		win::event m_start_evt;
		win::event m_stop_evt;
		win::critical_section m_cache_cs;
		std::vector<data_t::ptr> m_cache;
		const std::wstring m_instrument_id;
		const data_storage::ptr m_data_storage;
		const tbp::connector::ptr m_connector;
		std::thread m_instant_worker;
		std::thread m_historical_worker;

	private:
		void collect_instant_data_thread();
		void collect_historical_data_thread();
		void flush_cache();

	public:
		// SB: if data not present id data storage gets it from connector and updates data in storage 
		virtual std::vector<data_t::ptr> get_data(const std::wstring& instrument_id, unsigned long granularity, time_t* start_datetime, time_t* end_datetime) const override;
		virtual std::vector<data_t::ptr> get_instant_data(const std::wstring& instrument_id, time_t* start_datetime, time_t* end_datetime) const override;

	public:
		void start();

	public:
		data_collector(const std::wstring& instrument_id, const settings::ptr& s, const tbp::connector::ptr& connector, const data_storage::ptr& ds);
		~data_collector();
	};
}