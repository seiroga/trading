#pragma once

#include <core/connector.h>
#include <core/settings.h>

namespace tbp
{
	namespace oanda
	{
		/////////////////////////////////////////////////////////////////////////
		// authentication

		class authentication : public tbp::authentication
		{
			const std::wstring m_access_token;

		public:
			virtual void login() override;
			virtual std::wstring get_token() const override;

		public:
			authentication(const std::wstring& access_token);
			~authentication();
		};

		data_t create_market_order_params(const std::wstring& instrument_id, double amount);

		/////////////////////////////////////////////////////////////////////////
		// connector

		struct connector
		{
			static tbp::connector::ptr create(const settings::ptr& settings, const authentication::ptr& auth);
		};
	}
}