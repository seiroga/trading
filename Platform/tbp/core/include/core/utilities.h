#pragma once

#include <core/primitives.h>

namespace tbp
{
	template<typename duration_t, typename _Rep, typename _Period>
	auto align_to_granularity(const tbp::time_t& time, const std::chrono::duration<_Rep, _Period>& granularity)
	{
		return std::chrono::duration_cast<duration_t>((time.time_since_epoch() / granularity) * granularity);
	}
}