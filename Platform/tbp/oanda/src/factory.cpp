#include <oanda/factory.h>
#include <oanda/data_storage.h>
#include <oanda/connector.h>
#include <oanda/trader.h>

#include <common/string_cvt.h>

#include <sqlite/sqlite.h>
#include <logging/log.h>

#include <win/fs.h>

#include <cpprest/json.h>

#include <fstream>

namespace tbp
{
	namespace oanda
	{
		static settings::ptr load_settings(const std::wstring& path)
		{
			using win::fs::operator/;

			auto connector_info_path = path / L"connector_info.json";
			if (!win::fs::exists(connector_info_path))
			{
				throw std::runtime_error(sb::to_str(connector_info_path + L" file wasn't found!"));
			}

			const auto utf8_path = sb::to_str(connector_info_path);
			std::ifstream connector_info_file(utf8_path.c_str(), std::ios::in);
			if (!connector_info_file.is_open())
			{
				throw std::runtime_error("Unable to open connector_info.json file!");
			}

			auto s = settings::load_from_json(connector_info_file);

			LOG_INFO << L"Connector settings has been loaded successfully!";

			return s;
		}

		////////////////////////////////////////////////////////////////
		// factory

		tbp::authentication::ptr factory::create_auth()
		{
			return std::make_shared<authentication>(get_value<std::wstring>(m_connector_settings, L"auth_token"));
		}

		tbp::connector::ptr factory::create_connector(const authentication::ptr& auth)
		{
			return connector::create(m_connector_settings, auth);
		}

		data_storage::ptr factory::create_storage()
		{
			using win::fs::operator/;
			std::wstring full_path = m_working_dir / L"DB" / L"oanda";
			win::fs::create_path(full_path);

			full_path = full_path / L"instruments_data.db";
			if (win::fs::exists(full_path))
			{
				LOG_INFO << "Open existing OANDA database.";
			}
			else
			{
				LOG_INFO << "Create OANDA database.";
			}

			auto db = sqlite::connection::create(full_path);

			LOG_INFO << "OANDA database connection created successfully.";

			return std::make_shared<oanda::data_storage>(db);
		}

		tbp::trader::ptr factory::create_trader(const tbp::connector::ptr& c)
		{
			return std::make_shared<oanda::trader>(c, m_working_dir);
		}

		factory::ptr factory::create(const std::wstring& working_dir)
		{
			return std::make_shared<factory>(working_dir);
		}

		factory::factory(const std::wstring& working_dir)
			: m_working_dir(working_dir)
			, m_connector_settings(load_settings(working_dir))
		{
		}
	}
}