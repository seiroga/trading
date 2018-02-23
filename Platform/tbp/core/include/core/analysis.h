#pragma once

#include <boost/numeric/conversion/cast.hpp>

namespace tbp
{
	namespace analysis
	{
		template<typename values_container_t, typename ema_container_t>
		void calculate_ema(const values_container_t& values, ema_container_t* ema, const unsigned long ema_length)
		{
			using ema_value_t = typename ema_container_t::value_type;

			if (values.size() < ema_length || 0 == ema_length)
			{
				throw std::runtime_error("Not enought data to calculate EMA!");
			}

			if (ema->size() >= values.size())
			{
				throw std::runtime_error("EMA size greater than values size!");
			}

			const double alpha = 2.0 / (ema_length + 1);
			if (ema->size() < ema_length)
			{
				// SB: clear EMA values in case if we need to calculate SMA first
				ema->clear();
			}

			if (0 == ema->size())
			{
				auto sma = 0.0;
				size_t i = 0;
				for (const auto& val : values)
				{
					if (i < ema_length)
					{
						sma += val;
						ema->push_back(ema_value_t(0));

						if (ema_length - 1 == i)
						{
							sma /= ema_length;
							ema->back() = static_cast<ema_value_t>(sma);
						}
					}
					else
					{
						auto ema_val = val * alpha + (1.0 - alpha) * ema->back();
						ema->push_back(static_cast<ema_value_t>(ema_val));
					}

					++i;
				}
			}
			else
			{
				for (size_t i = ema->size(); i < values.size(); ++i)
				{
					auto ema_val = values[i] * alpha + (1.0 - alpha) * ema->back();
					ema->push_back(static_cast<ema_value_t>(ema_val));
				}
			}
		}
	}
}