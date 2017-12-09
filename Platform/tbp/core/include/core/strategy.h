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

	public:
		virtual void start() = 0;
		virtual void stop() = 0;
	};
}