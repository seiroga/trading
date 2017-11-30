#pragma once

#include <core/factory.h>
#include <core/settings.h>

#include <thread>
#include <memory>

namespace tbp
{
	class application
	{
		const factory::ptr m_factory;
		const settings::ptr m_settings;
		data_storage::ptr m_storage;
		connector::ptr m_connector;
		data_collector::ptr m_data_collector;
		std::shared_ptr<sb::dynamic> m_trader;
		std::unique_ptr<std::thread> m_ui_thread;

	private:
		void start_ui_thread();
		void pump() const;
		std::wstring get_working_instrument();

		static settings::ptr load_settings(const std::wstring& path);

	public:
		void start();

	public:
		application(const factory::ptr& f, const std::wstring& working_dir);
		~application();
	};
}