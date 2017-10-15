#pragma once

#include <core/connector.h>

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

		/////////////////////////////////////////////////////////////////////////
		// connector

		struct connector
		{
			static tbp::connector::ptr create(const authentication::ptr& auth, bool practice);
		};
	}
}