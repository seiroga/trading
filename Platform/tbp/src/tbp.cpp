#include "application.h"

#include <common/string_cvt.h>

#include <oanda/factory.h>
#include <core/factory.h>

#include <sqlite/sqlite.h>
#include <logging/log.h>

#include <win/fs.h>
#include <win/exception.h>

#include <iostream>

namespace tbp
{
	namespace
	{
		const std::map<std::wstring, tbp::factory::ptr(*)(const std::wstring& working_dir)> broker_factories = 
		{
			{ L"OANDA", &oanda::factory::create }
		};

		tbp::factory::ptr get_broker_factory(const std::wstring& broker_id, const std::wstring& working_dir)
		{
			auto it = broker_factories.find(broker_id);
			if (broker_factories.end() == it)
			{
				throw std::runtime_error("Factory for specified broker wasn't found!");
			}

			return it->second(working_dir);
		}
	}
}

int wmain()
{
	try
	{
		using win::fs::operator/;

		const std::wstring working_dir = win::fs::get_current_module_dir();
		logging::init(logging::level::debug, working_dir / L"Logs", true);

		tbp::application app(tbp::get_broker_factory(L"OANDA", working_dir));
		app.start();
	}
	catch (const std::exception& ex)
	{
		std::cout << "Exception was thrown during application initialization!. Info: " << ex.what();

		return -1;
	}
	catch (const win::exception& ex)
	{
		// SB: TODO need to implement string conversion function!!!
		std::cout << "Exception was thrown during application initialization!. Info: " << sb::to_str(ex.msg);

		return -1;
	}
	catch (...)
	{
		std::cout << "Unknown exception was thrown during application initialization!";

		return -2;
	}

	LOG_INFO << L"============================== logging finished ===================================";

    return 0;
}