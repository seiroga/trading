#include "application.h"

#include <logging/log.h>
#include <win/thread.h>
#include <win/fs.h>

#include <windows.h>

#include <fstream>

namespace tbp
{
	using win::fs::operator/;

	void application::start()
	{
		using win::fs::operator/;

		LOG_INFO << "Starting application...";

		m_storage = m_factory->create_storage();
		
		auto auth = m_factory->create_auth();
		auth->login();

		m_connector = m_factory->create_connector(auth);

		const auto working_instrument = get_working_instrument();

		m_data_collector = std::make_shared<tbp::data_collector>(working_instrument, m_settings, m_connector, m_storage);

		// SB: create strategy
		// ...........

		// SB: create trader
		m_trader = m_factory->create_trader(m_connector, nullptr /*<--- !!!! need to pass real strategy instead of nullptr !!!*/);

		m_data_collector->start();

		start_ui_thread();

		LOG_INFO << "Application started successfully!";

		pump();
	}

	std::wstring application::get_working_instrument()
	{
		const auto working_instrument = get_value<std::wstring>(m_settings, L"WorkingInstrument", L"EUR_USD");

		LOG_INFO << "Applicaiton configured to work with: " << working_instrument;
		LOG_INFO << "Verifying working instrument identifier...";

		auto instruments = m_connector->get_instruments();

		auto it = std::find(instruments.begin(), instruments.end(), working_instrument);
		if (instruments.end() == it)
		{
			throw std::runtime_error("Instrument " + sb::to_str(working_instrument) + " isn't supported by broker!");
		}

		LOG_INFO << "Working instrument verfied successfully!";

		return working_instrument;
	}

	void application::start_ui_thread()
	{
		const auto main_thread_id = ::GetCurrentThreadId();
		m_ui_thread = std::make_unique<std::thread>([main_thread_id]()
		{
			::MessageBoxW(0, L"Click Ok to close application!", L"Info", MB_OK | MB_ICONINFORMATION);

			// SB: post WM_QUIT to main thread
			::PostThreadMessageW(main_thread_id, WM_QUIT, 0, 0);

			LOG_INFO << "Application was stopped by user request!";
		});
	}

	void application::pump() const
	{
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

	settings::ptr application::load_settings(const std::wstring& path)
	{
		if (!win::fs::exists(path))
		{
			LOG_WARN << L"app_settings.json configuration file missing. Empty settings is used!";
		}

		return tbp::settings::load_from_json(std::ifstream(path));
	}

	application::application(const factory::ptr& f, const std::wstring& working_dir)
		: m_factory(f)
		, m_settings(load_settings(working_dir / L"app_settings.json"))
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