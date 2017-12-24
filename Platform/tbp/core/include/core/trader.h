#pragma once

#include <common/constrains.h>

#include <string>
#include <memory>
#include <exception>

namespace tbp
{
	struct trader : public sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<trader>;

		struct trade_canceled : public std::runtime_error
		{
			using runtime_error::runtime_error;
		};

	public:
		virtual std::wstring open_trade(const std::wstring& instrument_id, long amount) = 0;
		virtual void close_trade(const std::wstring& internal_id, long amount) = 0;
		virtual void close_pending_trades() = 0;
	};
}