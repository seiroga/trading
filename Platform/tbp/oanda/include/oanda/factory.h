#pragma once

#include <core/factory.h>

#include <memory>

namespace tbp
{
	namespace oanda
	{
		class factory : public tbp::factory
		{
			struct connector_info;

		private:
			const std::wstring m_working_dir;
			const std::unique_ptr<connector_info> m_connector_cfg;

		public:
			virtual tbp::authentication::ptr create_auth() override;
			virtual tbp::connector::ptr create_connector(const authentication::ptr& auth) override;
			virtual tbp::data_storage::ptr create_storage() override;

		public:
			static ptr create(const std::wstring& working_dir);

		public:
			factory(const std::wstring& working_dir);
		};
	}
}