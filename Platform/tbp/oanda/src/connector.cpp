#include <oanda/connector.h>

namespace tbp
{
	namespace oanda
	{
		/////////////////////////////////////////////////////////////////////////
		// authentication implemnetation

		void authentication::login()
		{
			// Do nothing. We work through predefined access token
		}

		std::wstring authentication::get_token() const
		{
			return m_access_token;
		}

		authentication::authentication(const std::wstring& access_token)
			: m_access_token(access_token)
		{
		}

		authentication::~authentication()
		{
		}

		/////////////////////////////////////////////////////////////////////////
		// connector implemnetation

		std::vector<data_t::ptr> conector::get_data(const std::wstring& instrument_id, time_t* start_datetime, time_t* end_datetime) const
		{
			return std::vector<data_t::ptr>();
		}

		std::vector<std::wstring> conector::get_instruments() const
		{
			return std::vector<std::wstring>();
		}

		data_t::ptr conector::get_instant_data(const std::wstring& instrument_id)
		{
			return data_t::ptr();
		}

		order::ptr conector::create_order(const data_t& params)
		{
			return order::ptr();
		}

		//conector::connector(const tbp::authentication::ptr& auth)
		//{
		//}
	}
}