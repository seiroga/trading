#include <sqlite/sqlite.h>
#include <logging/log.h>

#include <win/fs.h>

namespace tbp
{
}

int wmain()
{
	using win::fs::operator/;

	logging::init(logging::level::debug, win::fs::get_current_module_dir() / L"Logs", true);
	
	LOG_INFO << "Application started";
	LOG_INFO << "Application shutdown";

    return 0;
}