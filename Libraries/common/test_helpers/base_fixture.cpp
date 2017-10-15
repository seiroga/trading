#include <test_helpers/base_fixture.h>

#include <win/exception.h>

#include <vector>
#include <algorithm>
#include <memory>

#include <objbase.h>
#include <comdef.h>

#include <shlobj.h>

namespace test_helpers
{
	namespace
	{
		std::wstring generate_guid()
		{
			GUID guid = { 0 };
			if (S_OK != ::CoCreateGuid(&guid))
			{
				throw win::exception(L"CoCreateGuid call failed!");
			}

			auto co_task_mem_free = [](LPOLESTR ptr)
			{
				::CoTaskMemFree(ptr);
			};

			LPOLESTR guid_str = nullptr;
			if (S_OK != ::StringFromCLSID(guid, &guid_str))
			{
				throw win::exception(L"StringFromCLSID call failed!");
			}

			std::unique_ptr<OLECHAR, decltype(co_task_mem_free)> guid_ptr(guid_str, co_task_mem_free);
			std::wstring result(guid_str);

			return result;
		}

		std::wstring get_module_directory()
		{
			wchar_t path_buff[1024] = { 0 };
			if (0 == ::GetModuleFileNameW(nullptr, path_buff, _countof(path_buff)))
			{
				throw win::exception(L"GetModuleFileNameW call failed!");
			}

			std::wstring path(path_buff);
			size_t pos = path.find_last_of(L"\\");
			if (std::wstring::npos == pos)
			{
				throw std::runtime_error(__FUNCTION__ " Invalid path format!");
			}

			return path.substr(0, pos);
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	// temp_folder implementation

	std::wstring base_fixture::temp_folder::get_default_path()
	{
		return get_module_directory() + L"\\test_data";
	}

	base_fixture::temp_folder::temp_folder(const std::wstring& path /*= get_default_path()*/)
		: path(path)
	{
		int result = ::SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
		if (ERROR_SUCCESS != result && ERROR_FILE_EXISTS != result && ERROR_ALREADY_EXISTS != result)
			throw win::exception((L"Failed to create path: " + path).c_str(), result);
	}

	base_fixture::temp_folder::~temp_folder()
	{
		std::vector<wchar_t> path_with_double_terminator(path.length() + 2, L'\0');
		std::copy(path.begin(), path.end(), path_with_double_terminator.begin());
		SHFILEOPSTRUCTW shf_info = { 0 };
		shf_info.pFrom = path_with_double_terminator.data();
		shf_info.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI | FOF_SILENT;
		shf_info.wFunc = FO_DELETE;
		int res = ::SHFileOperationW(&shf_info);
	}

	//////////////////////////////////////////////////////////////////////////////
	// base_fixture implementation

	std::wstring base_fixture::unique_string()
	{
		return generate_guid();
	}
}