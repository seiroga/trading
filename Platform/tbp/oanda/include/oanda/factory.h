#pragma once

#include <core/factory.h>
#include <core/settings.h>

#include <memory>

namespace tbp
{
	namespace oanda
	{
		class factory : public tbp::factory
		{
		private:
			const std::wstring m_working_dir;
			const settings::ptr m_connector_settings;

		public:
			virtual tbp::authentication::ptr create_auth() override;
			virtual tbp::connector::ptr create_connector(const authentication::ptr& auth) override;
			virtual tbp::data_storage::ptr create_storage() override;
			virtual tbp::trader::ptr create_trader(const tbp::connector::ptr& c) override;

		public:
			static ptr create(const std::wstring& working_dir);

		public:
			factory(const std::wstring& working_dir);
		};
	}
}