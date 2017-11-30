#pragma once

#include <common/constrains.h>

#include <boost/signals2.hpp>

#include <memory>
#include <string>

namespace tbp
{
	struct strategy : public sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<strategy>;
		using trade_id_t = std::wstring;

	public:
		boost::signals2::signal<void (const std::wstring& instrument_id, long amount, trade_id_t trade_id)> on_open_trade;
		boost::signals2::signal<void (trade_id_t trade_id, long amount)> on_close_trade;

	public:
		virtual void start() = 0;
		virtual void stop() = 0;
	};
}