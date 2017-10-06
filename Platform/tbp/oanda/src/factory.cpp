#include <oanda/factory.h>
#include <oanda/data_storage.h>
#include <oanda/connector.h>

#include <sqlite/sqlite.h>
#include <logging/log.h>

#include <win/fs.h>

#include <locale>
#include <codecvt>
#include <fstream>

namespace tbp
{
	namespace oanda
	{
		authentication::ptr factory::create_auth()
		{
			using win::fs::operator/;

			auto path = win::fs::get_current_module_dir() / L"auth.dat";
			if (!win::fs::exists(path))
			{
				throw std::runtime_error("Authentication token file wasn't found!");
			}

			std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> string_cvt;
			auto utf8_path = string_cvt.to_bytes(path);

			std::string token;
			std::ifstream auth_file(utf8_path.c_str(), std::ios::in);
			if (!auth_file.is_open())
			{
				throw std::runtime_error("Unable to open authentication token file!");
			}

			auth_file >> token;

			LOG_INFO << L"Authorization token has been read successfully!";

			return std::make_shared<authentication>(string_cvt.from_bytes(token));
		}

		connector::ptr factory::create_connector(const authentication::ptr& auth)
		{
			return connector::ptr();
		}

		data_storage::ptr factory::create_storage()
		{
			using win::fs::operator/;
			std::wstring full_path = m_working_dir / L"DB" / L"oanda";
			win::fs::create_path(full_path);

			full_path = full_path / L"instrument_data.db";
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
		{
		}
	}
}