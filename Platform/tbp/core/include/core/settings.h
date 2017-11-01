#pragma once

#include <common/constrains.h>

#include <boost/variant.hpp>

#include <memory>

namespace tbp
{
	struct empty_value {};

	using setting_value = boost::variant<empty_value, int, double, bool, std::wstring>;

	struct settings : public sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<settings>;

	public:
		virtual setting_value get(const std::wstring& name) = 0;
		virtual void set(const std::wstring& name, const setting_value& value) = 0;

	public:
		static settings::ptr load_from_json(const std::wstring& json_str);
		static settings::ptr load_from_json(std::ifstream& json_stream);
	};

	template<typename T>
	T get_value(const settings::ptr& s, const std::wstring& name)
	{
		return boost::get<T>(s->get(name));
	}

	template<typename T>
	T get_value(const settings::ptr& s, const std::wstring& name, T default_val)
	{
		auto setting_val = s->get(name);
		auto val = boost::get<empty_value>(&setting_val);
		if (nullptr != val)
		{
			return default_val;
		}

		return get_value<T>(s, name);
	}
}