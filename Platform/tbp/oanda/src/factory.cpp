#include <oanda/factory.h>
#include <oanda/data_storage.h>
#include <oanda/connector.h>

#include <sqlite/sqlite.h>
#include <logging/log.h>

#include <win/fs.h>

namespace tbp
{
	namespace oanda
	{
		authentication::ptr factory::create_auth()
		{
			return authentication::ptr();
		}

		connector::ptr factory::create_connector(const authentication::ptr& auth)
		{
			return connector::ptr();
		}

		data_storage::ptr factory::create_storage(const std::wstring& path)
		{
			using win::fs::operator/;
			std::wstring full_path = path / L"oanda";
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

		factory::ptr factory::create()
		{
			return std::make_shared<factory>();
		}
	}
}