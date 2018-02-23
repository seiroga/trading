#pragma once

#include <core/primitives.h>

#include <common/constrains.h>

#include <string>
#include <memory>
#include <exception>

namespace tbp
{
	struct candle_info
	{
		double high = 0.0;
		double open = 0.0;
		double close = 0.0;
		double low = 0.0;
	};

	struct candlestick_data
	{
		candle_info bid;
		candle_info ask;
		int volume = 0;
		time_t timestamp = time_t();
		bool complete = true;

		static double get_middle(const candle_info& ci)
		{
			return (ci.high + ci.low) / 2;
		}
	};

	struct trader : public sb::dynamic
	{
	public:
		using ptr = std::shared_ptr<trader>;

		struct trade_canceled : public std::runtime_error
		{
			using runtime_error::runtime_error;
		};

	public:
		virtual std::wstring open_trade(const std::wstring& instrument_id, double amount) = 0;
		virtual void close_trade(const std::wstring& internal_id, double amount) = 0;
		virtual void close_pending_trades() = 0;

		// SB: connector get_data and get_instant_data should return vector of candlestick_data instead of vector of data_t::ptr
		virtual std::vector<candlestick_data> get_candles_from_data(const std::vector<data_t::ptr>& candles_data) const = 0;
	};
}