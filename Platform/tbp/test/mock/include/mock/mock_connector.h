#pragma once

#include <test_helpers/base_fixture.h>

#include <core/connector.h>

#include <string>

////////////////////////////////////////////////////////////////////////
// mock_trade

struct mock_trade : public tbp::trade
{
	std::wstring remote_id;
	state_t current_state;
	long trading_amount;
	double profit;

public:
	virtual std::wstring id() const override
	{
		return remote_id;
	}

	virtual state_t state() const override
	{
		return current_state;
	}

	virtual long amount() const override
	{
		return trading_amount;
	}

	virtual double current_profit() const override
	{
		return profit;
	}

	virtual void close(unsigned long amount_to_close = -1L) override
	{
		current_state = state_t::closed;
	}

public:
	mock_trade(const std::wstring& id, state_t st)
		: remote_id(id)
		, current_state(st)
	{
	}
};

////////////////////////////////////////////////////////////////////////
// mock_order

struct mock_order : public tbp::order
{
	tbp::data_t params;
	std::wstring remote_id;
	std::wstring linked_trade_id;
	state_t current_state;

public:
	void fill_order()
	{
		linked_trade_id = test_helpers::base_fixture::unique_string();
		current_state = state_t::filled;
	}

public:
	virtual std::wstring id() const override
	{
		return remote_id;
	}

	virtual std::wstring trade_id() const override
	{
		return linked_trade_id;
	}

	virtual state_t state() const override
	{
		return current_state;
	}

	virtual void cancel() override
	{
		linked_trade_id.clear();
		current_state = state_t::canceled;
	}

public:
	mock_order(const tbp::data_t& p)
		: params(p)
		, remote_id(test_helpers::base_fixture::unique_string())
		, current_state(state_t::pending)
	{
	}

	mock_order(const std::wstring& id, const std::wstring& trade_id, state_t st)
		: remote_id(id)
		, linked_trade_id(trade_id)
		, current_state(st)
	{
	}
};

////////////////////////////////////////////////////////////////////////
// mock_connector

struct mock_connector : public tbp::connector
{
	struct request_info
	{
		const std::wstring instrument_id;
		const tbp::time_t start;
		const tbp::time_t end;
	};

	tbp::data_t::ptr value;
	mutable std::vector<request_info> data_request_log;
	mutable std::vector<std::wstring> instant_data_request_log;
	std::vector<std::shared_ptr<mock_order>> orders_log;
	std::vector<std::shared_ptr<mock_trade>> trades_log;

	virtual std::vector<std::wstring> get_instruments() const override
	{
		return { L"" };
	}

	virtual tbp::data_t::ptr get_instant_data(const std::wstring& instrument_id) override
	{
		instant_data_request_log.emplace_back(instrument_id);

		return value;
	}

	virtual std::vector<tbp::data_t::ptr> get_data(const std::wstring& instrument_id, tbp::time_t* start_datetime, tbp::time_t* end_datetime) const override
	{
		data_request_log.push_back({ instrument_id, *start_datetime, *end_datetime });

		return { value };
	}

	virtual tbp::order::ptr create_order(const tbp::data_t& params) override
	{
		auto mo = std::make_shared<mock_order>(params);
		orders_log.push_back(mo);

		return mo;
	}

	virtual tbp::order::ptr find_order(const std::wstring& id) const override
	{
		for (const auto& order : orders_log)
		{
			if (id == order->remote_id)
			{
				return order;
			}
		}

		return nullptr;
	}

	virtual tbp::trade::ptr find_trade(const std::wstring& id) const override
	{
		return nullptr;
	}
};