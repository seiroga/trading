#pragma once

#include <string>

#include <windows.h>

namespace win
{
	namespace com
	{
		GUID generate_guid();
		std::wstring guid_to_str(const GUID& guid);
	}
}