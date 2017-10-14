#pragma once

#include <string>
#include <locale>
#include <codecvt>
#include <type_traits>

namespace sb
{
	template<typename char_t>
	struct str_cvt;

	template<>
	struct str_cvt<char>
	{
		template<typename cvt_t>
		static auto to_str(const std::string& str, cvt_t& cvt)
		{
			return cvt.from_bytes(str);
		}
	};

	template<>
	struct str_cvt<wchar_t>
	{
		template<typename cvt_t>
		static auto to_str(const std::wstring& str, cvt_t& cvt)
		{
			return cvt.to_bytes(str);
		}
	};

	template<typename char_t>
	auto to_str(const std::basic_string<char_t>& str)
	{
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> cvt;

		return str_cvt<char_t>::to_str(str, cvt);
	}
}