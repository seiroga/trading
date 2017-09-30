#pragma once

#include <core/factory.h>

namespace tbp
{
	namespace oanda
	{
		class factory : public tbp::factory
		{
		public:
			virtual authentication::ptr create_auth() override;
			virtual connector::ptr create_connector(const authentication::ptr& auth) override;
			virtual data_storage::ptr create_storage(const std::wstring& path) override;

		public:
			static ptr create();
		};
	}
}