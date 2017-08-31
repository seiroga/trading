#include <win/fs.h>
#include <win/exception.h>

#include <string>

#include <windows.h>

namespace win
{
	namespace fs
	{
		std::wstring operator/(std::wstring lhs, std::wstring rhs)
		{
			const auto last_char = *lhs.rbegin();
			if (last_char != L'\\' || last_char != L'/')
			{
				return lhs + L"\\" + rhs;
			}

			return lhs + rhs;
		}

		std::wstring get_current_module_dir()
		{
			wchar_t buf[1024] = { 0 };
			auto res = ::GetModuleFileNameW(0, &buf[0], _countof(buf));
			if (0 == res)
				throw win::exception(L"GetModuleFileNameW call failed!");

			std::wstring path(buf);
			return path.substr(0, path.find_last_of(L"\\"));
		}
	}
}