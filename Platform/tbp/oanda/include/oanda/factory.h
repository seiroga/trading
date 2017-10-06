#pragma once

#include <core/factory.h>

namespace tbp
{
	namespace oanda
	{
		class factory : public tbp::factory
		{
			const std::wstring m_working_dir;

		public:
			virtual authentication::ptr create_auth() override;
			virtual connector::ptr create_connector(const authentication::ptr& auth) override;
			virtual data_storage::ptr create_storage() override;

		public:
			static ptr create(const std::wstring& working_dir);

		public:
			factory(const std::wstring& working_dir);
		};
	}
}