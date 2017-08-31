#pragma once

#include <string>

namespace win
{
	namespace fs
	{
		std::wstring operator/(std::wstring lhs, std::wstring rhs);

		std::wstring get_current_module_dir();
	}
}