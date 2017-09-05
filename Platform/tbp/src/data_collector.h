#pragma once

#include <tbp/data_storage.h>
#include <tbp/connector.h>

#include<win/thread.h>

#include <thread>
#include <string>

namespace tbp
{
	class data_collector
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
		data_collector(const std::wstring& instrument_id, const tbp::connector::ptr& connector, const data_storage::ptr& ds);
		~data_collector();
	};
}