#pragma once

#include <windows.h>

namespace win
{
	struct exception
	{
		const DWORD code;
		const std::wstring msg;

		exception(const std::wstring& msg, DWORD err_code = ::GetLastError())
			: code(err_code)
			, msg(msg)
		{
		}
	};
}