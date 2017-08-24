#include <sqlite/sqlite.h>
#include <logging/log.h>

#include <win/exception.h>

class data_provider
{
public:
	virtual sqlite::connection::ptr create_data_provider() = 0;
};

std::wstring get_current_module_dir()
{
	wchar_t buf[1024] = { 0 };
	auto res = ::GetModuleFileNameW(0, &buf[0], _countof(buf));
	if (0 == res)
		throw win::exception(L"GetModuleFileNameW call failed!");

	std::wstring path(buf);
	return path.substr(0, path.find_last_of(L"\\"));
}

int wmain()
{
	logging::init(logging::level::debug, get_current_module_dir() + L"\\Logs\\", true);
	
	LOG_INFO << "Application started";
	LOG_INFO << "Application shutdown";

    return 0;
}