#include "application.h"

#include <logging/log.h>
#include <win/thread.h>
#include <win/fs.h>

#include <windows.h>

namespace tbp
{
	void application::start()
	{
		using win::fs::operator/;

		LOG_INFO << "Starting application...";

		m_storage = m_factory->create_storage();
		
		auto auth = m_factory->create_auth();
		auth->login();

		m_connector = m_factory->create_connector(auth);

		//auto start = std::chrono::system_clock::now() - std::chrono::hours(1);
		//auto end = std::chrono::system_clock::now();
		//m_connector->get_data(L"EUR_USD", &start, &end);

		//m_connector->get_instruments();
		//m_connector->get_instant_data(L"EUR_USD");

		m_data_collector = std::make_shared<tbp::data_collector>(L"EUR_USD", m_connector, m_storage);

		const auto main_thread_id = ::GetCurrentThreadId();
		m_ui_thread = std::make_unique<std::thread>([main_thread_id]()
		{
			::MessageBoxW(0, L"Click Ok to close application!", L"Info", MB_OK | MB_ICONINFORMATION);

			// SB: post WM_QUIT to main thread
			::PostThreadMessageW(main_thread_id, WM_QUIT, 0, 0);

			LOG_INFO << "Application was stopped by user request!";
		});

		LOG_INFO << "Application started successfully!";

		BOOL res = 0;
		MSG msg = { 0 };
		while (0 != (res = ::GetMessageW(&msg, 0, 0, 0)))
		{
			if (-1 == res)
			{
				throw win::exception(L"GetMessageW call failed!");
			}

			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}
	}

	application::application(const factory::ptr& f)
		: m_factory(f)
	{
	}

	application::~application()
	{
		LOG_INFO << "Begin application shutdown!";

		if (nullptr != m_ui_thread)
		{
			m_ui_thread->join();
		}

		LOG_INFO << "Application has been shutdown!";
	}
}