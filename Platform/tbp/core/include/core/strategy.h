#pragma once

#include <core/connector.h>
#include <core/data_storage.h>
#include <core/trader.h>
#include <core/settings.h>

#include <common/constrains.h>

#include <memory>
#include <string>

namespace tbp
{
	struct strategy : public sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<strategy>;
	};

	struct pattern_base
	{
	public:
		using ptr = std::shared_ptr<pattern_base>;

	public:
		virtual bool check(const std::vector<candlestick_data>& data) = 0;
	};

	strategy::ptr create_ema_strategy(const data_provider::ptr& dp, const connector::ptr& c, const trader::ptr& t, const settings::ptr& s);
}