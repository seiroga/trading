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

		class conector : public tbp::connector
		{
		public:
			virtual std::vector<data_t::ptr> get_data(const std::wstring& instrument_id, time_t* start_datetime, time_t* end_datetime) const override;

		public:
			virtual std::vector<std::wstring> get_instruments() const override;
			virtual data_t::ptr get_instant_data(const std::wstring& instrument_id) override;
			virtual order::ptr create_order(const data_t& params) override;

		//public:
		//	connector(const authentication::ptr& auth);
		};
	}
}