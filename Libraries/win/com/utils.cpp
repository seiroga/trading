#include <win/com/utils.h>
#include <win/exception.h>

#include <memory>

namespace win
{
	namespace com
	{
		GUID generate_guid()
		{
			GUID guid = { 0 };
			if (S_OK != ::CoCreateGuid(&guid))
			{
				throw win::exception(L"CoCreateGuid call failed!");
			}

			return guid;
		}

		std::wstring guid_to_str(const GUID& guid)
		{
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
	}
}