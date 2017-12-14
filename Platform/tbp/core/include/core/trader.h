#pragma once

#include <common/constrains.h>

#include <string>
#include <memory>

namespace tbp
{
	struct trader : public sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<trader>;

	public:
		virtual void open_trade(const std::wstring& instrument_id, long amount, const std::wstring& internal_id) = 0;
		virtual void close_trade(const std::wstring& internal_id, long amount) = 0;
		virtual void close_pending_trades() = 0;
	};
}