#include <oanda/factory.h>
#include <oanda/data_storage.h>
#include <oanda/connector.h>

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
		////////////////////////////////////////////////////////////////
		// connector_info

		struct factory::connector_info
		{
			const std::wstring url;
			const std::wstring auth_token;
			const std::wstring account_id;

		public:
			static std::unique_ptr<connector_info> load(const std::wstring& path)
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

				auto oanda_cfg_root = web::json::value::parse(connector_info_file).at(L"connector_info").at(L"OANDA");
				std::wstring url = oanda_cfg_root.at(L"url").as_string();
				std::wstring auth_token = oanda_cfg_root.at(L"auth_token").as_string();
				std::wstring account_id = oanda_cfg_root.at(L"account_id").as_string();

				LOG_INFO << L"Connector info has been loaded successfully!";

				return std::make_unique<connector_info>(connector_info{ url, auth_token, account_id });
			}
		};

		////////////////////////////////////////////////////////////////
		// factory

		tbp::authentication::ptr factory::create_auth()
		{
			return std::make_shared<authentication>(m_connector_cfg->auth_token);
		}

		tbp::connector::ptr factory::create_connector(const authentication::ptr& auth)
		{
			return connector::create(m_connector_cfg->url, m_connector_cfg->account_id, auth);
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

		factory::ptr factory::create(const std::wstring& working_dir)
		{
			return std::make_shared<factory>(working_dir);
		}

		factory::factory(const std::wstring& working_dir)
			: m_working_dir(working_dir)
			, m_connector_cfg(connector_info::load(working_dir))
		{
		}
	}
}