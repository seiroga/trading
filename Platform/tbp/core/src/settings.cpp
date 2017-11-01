#include <core/settings.h>

#include <win/fs.h>

// SB: to minimize frameworks to use I suggest to use cpprest json library
// but it should be some another more light lib
#include <cpprest/json.h>

namespace tbp
{
	namespace
	{
		///////////////////////////////////////////////////////////////////////////////////////
		// json_settings_impl

		class json_settings_impl : public settings
		{
			web::json::value m_settings;

		public:
			virtual setting_value get(const std::wstring& name) override
			{
				auto value = m_settings.at(name);
				switch (value.type())
				{
				case web::json::value::value_type::Number:
					{
						auto val = value.as_number();
						if (val.is_integral())
						{
							return val.to_int32();
						}
						else
						{
							return val.to_double();
						}
					}

				case web::json::value::value_type::Boolean:
					{
						return value.as_bool();
					}

				case web::json::value::value_type::String:
					{
						return value.as_string();
					}

				default:
					throw std::runtime_error("Unsupported settings value!");
				}
			}

			virtual void set(const std::wstring& name, const setting_value& value) override
			{
				throw std::runtime_error("Not implemented!");
			}

		public:
			json_settings_impl(const std::wstring& cfg_str)
				: m_settings(web::json::value::parse(cfg_str))
			{
			}

			json_settings_impl(std::ifstream& cfg_stream)
				: m_settings(web::json::value::parse(cfg_stream))
			{
			}

			json_settings_impl()
				: m_settings(web::json::value())
			{
			}
		};
	}

	////////////////////////////////////////////////////////////////////////////////
	// settings

	settings::ptr settings::load_from_json(const std::wstring& json_str)
	{
		return std::make_shared<json_settings_impl>(json_str);
	}

	settings::ptr settings::load_from_json(std::ifstream& json_stream)
	{
		return std::make_shared<json_settings_impl>(json_stream);
	}
}