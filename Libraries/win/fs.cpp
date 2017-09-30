#include <win/fs.h>
#include <win/exception.h>

#include <string>

#include <windows.h>
#include <shlobj.h>

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

		bool exists(const std::wstring& path)
		{
			auto res = ::GetFileAttributesW(path.c_str());
			if (INVALID_FILE_ATTRIBUTES == res)
			{
				auto last_err = ::GetLastError();
				if (ERROR_FILE_NOT_FOUND == last_err)
				{
					return false;
				}

				throw win::exception(L"GetFileAttributesW call failed!", last_err);
			}

			return true;
		}

		void create_path(const std::wstring& path)
		{
			int result = ::SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
			if (ERROR_SUCCESS != result && ERROR_FILE_EXISTS != result && ERROR_ALREADY_EXISTS != result)
				throw win::exception(L"SHCreateDirectoryExW call failed!");
		}
	}
}