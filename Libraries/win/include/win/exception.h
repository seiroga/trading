#pragma once

#include <string>

#include <windows.h>

namespace win
{
	struct exception
	{
		const DWORD code;
		const std::wstring msg;

		exception(const wchar_t* msg, DWORD err_code = ::GetLastError())
			: code(err_code)
			, msg(msg)
		{
		}
	};
}