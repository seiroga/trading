#pragma once

#include <core/factory.h>

#include <thread>
#include <memory>

namespace tbp
{
	class application
	{
		factory::ptr m_factory;
		data_storage::ptr m_storage;
		connector::ptr m_connector;
		data_collector::ptr m_data_collector;
		std::unique_ptr<std::thread> m_ui_thread;

	public:
		void start();

	public:
		application(const factory::ptr& f);
		~application();
	};
}