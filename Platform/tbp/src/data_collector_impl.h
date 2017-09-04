#pragma once

#include <tbp/data_collector.h>
#include <tbp/connector.h>

#include <sqlite/sqlite.h>

#include<win/thread.h>

#include <thread>
#include <string>

namespace tbp
{
	class data_collector_impl : public tbp::data_collector
	{
		win::event m_start_evt;
		win::event m_stop_evt;
		const std::wstring m_instrument_id;
		const data_storage::ptr m_data_storage;
		const tbp::connector::ptr m_connector;
		std::thread m_worker;

	private:
		void collect_data_thread();

	public:
		virtual data_provider::ptr get_data_provider() override;

	public:
		data_collector_impl(const std::wstring& instrument_id, const tbp::connector::ptr& connector, const data_storage::ptr& ds);
		~data_collector_impl();
	};
}