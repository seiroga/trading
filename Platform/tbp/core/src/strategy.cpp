#include <core/strategy.h>
#include <core/utilities.h>
#include <core/analysis.h>
#include <logging/log.h>

#include <boost/numeric/conversion/cast.hpp>

#include <queue>
#include <deque>

namespace tbp
{
	namespace
	{
		class ema_strategy_impl : public strategy
		{
			const std::chrono::seconds m_data_granularity;
			const std::chrono::seconds m_trend_interval;
			const std::wstring m_working_instrument;
			const data_provider::ptr m_data_provider;
			const connector::ptr m_connector;
			const trader::ptr m_trader;
			double m_margin_rate;
			std::deque<candlestick_data> m_trade_frame;
			std::deque<double> m_fast_ema_frame;
			std::deque<double> m_slow_ema_frame;
			std::wstring m_opened_trade_id;
			boost::signals2::connection m_historical_data_connection;

			double m_cross_value;
			bool m_waiting_for_threshold;

		private:
			void calculate_fast_ema(const std::vector<double>& values)
			{
				analysis::calculate_ema(values, &m_fast_ema_frame, 9);
			}

			void calculate_slow_ema(const std::vector<double>& values)
			{
				analysis::calculate_ema(values, &m_slow_ema_frame, 26);
			}

			double margin_rate()
			{
				if (0.0 != m_margin_rate)
				{
					return m_margin_rate;
				}

				m_margin_rate = m_connector->margin_rate();

				return m_margin_rate;
			}

			void init_historical_data()
			{
				tbp::time_t end_time(align_to_granularity<tbp::time_t::duration>(tbp::time_t::clock::now(), m_data_granularity));
				auto start_time = end_time - m_trend_interval;
				auto historical_data = m_data_provider->get_data(m_working_instrument, boost::numeric_cast<unsigned long>(m_data_granularity.count()), &start_time, &end_time);
				auto candles = m_trader->get_candles_from_data(historical_data);
				m_trade_frame.assign(candles.begin(), candles.end());

				std::vector<double> ask_values;
				ask_values.reserve(m_trade_frame.size());
				for (const auto& candle : m_trade_frame)
				{
					ask_values.push_back(candle.ask.close);
				}

				calculate_fast_ema(ask_values);
				calculate_slow_ema(ask_values);
			}

			void on_historical_data(const std::wstring& instrument_id, const std::vector<data_t::ptr>& data)
			{
				auto candles = m_trader->get_candles_from_data(data);
				if (candles.size() >= 2)
				{
					LOG_INFO << __FUNCTIONW__ << L" Arrived candles count is: " << candles.size();

					// SB: can happen f.e. if request failed several times
					while(candles.size() >= 2)
					{
						auto candle = candles.back();
						if (candle.complete)
						{
							candles.assign(1, candle);
						}

						candles.pop_back();
					}
				}

				m_trade_frame.pop_front();
				m_trade_frame.insert(m_trade_frame.end(), candles.begin(), candles.end());

				std::vector<double> ask_values;
				ask_values.reserve(m_trade_frame.size());
				for (const auto& candle : m_trade_frame)
				{
					ask_values.push_back(candle.ask.close);
				}

				// SB: remove the oldest EMA values
				m_fast_ema_frame.pop_front();
				m_slow_ema_frame.pop_front();

				calculate_fast_ema(ask_values);
				calculate_slow_ema(ask_values);

				// SB: try to find latest crossing
				//for (auto i = m_fast_ema_frame.size() - 1; i >= m_fast_ema_frame.size() - candles.size() - 1; --i)
				{
					auto i = m_fast_ema_frame.size() - 1;
					auto fast_val = m_fast_ema_frame[i];
					auto fast_prev_val = m_fast_ema_frame[i - 1];

					auto slow_val = m_slow_ema_frame[i];
					auto slow_prev_val = m_slow_ema_frame[i - 1];

					auto open_trade = [&](bool sell) 
					{
						if (!m_opened_trade_id.empty())
						{
							m_trader->close_trade(m_opened_trade_id, 0.0);
							m_opened_trade_id.clear();
						}

						auto available_money = m_connector->available_balance();
						auto margin = margin_rate();
						double trade_amount = long(available_money / margin / 2.0); // SB: <- 2.0 should be replaced with value from settings, which specifies risk level
						
						if (sell)
						{
							trade_amount *= -1.0;
						}

						LOG_DBG << L"EMA strategy" << (sell ? L" Sell " : L" Buy ") << trade_amount << L" " + m_working_instrument;

						m_opened_trade_id = m_trader->open_trade(m_working_instrument, trade_amount);
					};

					if (fast_prev_val <= slow_prev_val && fast_val > slow_val)
					{
						m_cross_value = (fast_val + fast_prev_val) / 2.0;
						m_waiting_for_threshold = true;

						LOG_DBG << L"EMA crossing detected. Growing. Cross value: " << m_cross_value;
					}
					else if (fast_prev_val >= slow_prev_val && fast_val < slow_val)
					{
						m_cross_value = (fast_val + fast_prev_val) / 2.0;
						m_waiting_for_threshold = true;

						LOG_DBG << L"EMA crossing detected. Falling. Cross value: " << m_cross_value;
					}

					const double threshold_value = abs(candles.back().ask.close - candles.back().bid.close) / 2.0;
					const double curr_diff = fast_val - m_cross_value;
					if (m_waiting_for_threshold && abs(curr_diff) >= threshold_value)
					{
						m_waiting_for_threshold = false;

						LOG_DBG << L"Threshold reached. Threshold value is: " << threshold_value;

						// SB: open trade
						open_trade(curr_diff < 0.0);
					}
				}
			}

		public:
			ema_strategy_impl(const data_provider::ptr& dp, const connector::ptr& c, const trader::ptr& t, const settings::ptr& s)
				: m_data_granularity(tbp::get_value<int>(s, L"DataGranularity", 60 /*1 minute*/))
				, m_trend_interval(tbp::get_value<int>(s, L"TradeFrame", 3600 /*1 hour*/))
				, m_working_instrument(tbp::get_value<std::wstring>(s, L"WorkingInstrument"))
				, m_data_provider(dp)
				, m_connector(c)
				, m_trader(t)
				, m_margin_rate(0.0)
				, m_historical_data_connection(m_data_provider->on_historical_data.connect(std::bind(&ema_strategy_impl::on_historical_data, this, std::placeholders::_1, std::placeholders::_2)))
				, m_cross_value(0.0)
				, m_waiting_for_threshold(false)
			{
				init_historical_data();
			}

			~ema_strategy_impl()
			{
				m_historical_data_connection.disconnect();
			}
		};
	}

	strategy::ptr create_ema_strategy(const data_provider::ptr& dp, const connector::ptr& c, const trader::ptr& t, const settings::ptr& s)
	{
		return std::make_shared<ema_strategy_impl>(dp, c, t, s);
	}
}