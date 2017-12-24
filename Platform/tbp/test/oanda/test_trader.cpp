#include <boost/test/unit_test.hpp>

#include <oanda/trader.h>

#include <mock/mock_connector.h>

#include <test_helpers/base_fixture.h>

#include <algorithm>
#include <functional>
#include <fstream>

namespace
{
	struct common_fixture : test_helpers::temp_dir_fixture
	{
	public:
		common_fixture()
			: temp_dir_fixture(L"")
		{
		}
	};
}

BOOST_FIXTURE_TEST_CASE(trader_creation, common_fixture)
{
	// INIT
	temp_folder working_dir;
	mock_connector::ptr connector = std::make_shared<mock_connector>();

	// ACT / ASSERT
	tbp::oanda::trader trader(connector, working_dir.path);
}

BOOST_FIXTURE_TEST_CASE(trader_open_close_trade, common_fixture)
{
	// INIT
	temp_folder working_dir;
	mock_connector::ptr connector = std::make_shared<mock_connector>();
	connector->fill_order_after_creation = true;
	tbp::oanda::trader trader(connector, working_dir.path);

	// ACT
	const auto trade_id = trader.open_trade(L"EUR_USD", 2000);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->orders_log.back()->current_state == tbp::order::state_t::filled);
	BOOST_ASSERT(!connector->orders_log.back()->linked_trade_id.empty());

	BOOST_ASSERT(connector->trades_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.back()->remote_id == connector->orders_log.back()->linked_trade_id);
	BOOST_ASSERT(connector->trades_log.back()->current_state == tbp::trade::state_t::opened);

	// ACT
	trader.close_trade(trade_id, -1L);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.back()->current_state == tbp::trade::state_t::closed);
}

BOOST_FIXTURE_TEST_CASE(trader_close_already_closed_trade, common_fixture)
{
	// INIT
	temp_folder working_dir;
	mock_connector::ptr connector = std::make_shared<mock_connector>();
	connector->fill_order_after_creation = true;
	tbp::oanda::trader trader(connector, working_dir.path);

	// ACT
	const auto trade_id = trader.open_trade(L"EUR_USD", 2000);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->orders_log.back()->current_state == tbp::order::state_t::filled);
	BOOST_ASSERT(!connector->orders_log.back()->linked_trade_id.empty());

	BOOST_ASSERT(connector->trades_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.back()->remote_id == connector->orders_log.back()->linked_trade_id);
	BOOST_ASSERT(connector->trades_log.back()->current_state == tbp::trade::state_t::opened);

	// ACT
	trader.close_trade(trade_id, -1L);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.back()->current_state == tbp::trade::state_t::closed);

	// ACT
	trader.close_trade(trade_id, -1L);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.back()->current_state == tbp::trade::state_t::closed);
}

BOOST_FIXTURE_TEST_CASE(trader_close_externally_closed_trade, common_fixture)
{
	// INIT
	temp_folder working_dir;
	mock_connector::ptr connector = std::make_shared<mock_connector>();
	connector->fill_order_after_creation = true;
	tbp::oanda::trader trader(connector, working_dir.path);

	// ACT
	const auto trade_id = trader.open_trade(L"EUR_USD", 2000);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->orders_log.back()->current_state == tbp::order::state_t::filled);
	BOOST_ASSERT(!connector->orders_log.back()->linked_trade_id.empty());

	BOOST_ASSERT(connector->trades_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.back()->remote_id == connector->orders_log.back()->linked_trade_id);
	BOOST_ASSERT(connector->trades_log.back()->current_state == tbp::trade::state_t::opened);

	// ACT
	connector->trades_log.back()->current_state = tbp::trade::state_t::closed;
	trader.close_trade(trade_id, -1L);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.size() == 1);
	BOOST_ASSERT(connector->trades_log.back()->current_state == tbp::trade::state_t::closed);
}

BOOST_FIXTURE_TEST_CASE(trader_open_cancel_trade, common_fixture)
{
	// INIT
	temp_folder working_dir;
	mock_connector::ptr connector = std::make_shared<mock_connector>();
	connector->fill_order_after_creation = false;
	tbp::oanda::trader trader(connector, working_dir.path);

	// ACT
	const auto trade_id = trader.open_trade(L"EUR_USD", 2000);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->orders_log.back()->current_state == tbp::order::state_t::pending);
	BOOST_ASSERT(connector->orders_log.back()->linked_trade_id.empty());

	BOOST_ASSERT(connector->trades_log.size() == 0);

	// ACT
	trader.close_trade(trade_id, -1L);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->orders_log.back()->current_state == tbp::order::state_t::canceled);
	BOOST_ASSERT(connector->trades_log.size() == 0);
}

BOOST_FIXTURE_TEST_CASE(trader_cancel_already_cancelled_trade, common_fixture)
{
	// INIT
	temp_folder working_dir;
	mock_connector::ptr connector = std::make_shared<mock_connector>();
	connector->fill_order_after_creation = false;
	tbp::oanda::trader trader(connector, working_dir.path);

	// ACT
	const auto trade_id = trader.open_trade(L"EUR_USD", 2000);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->orders_log.back()->current_state == tbp::order::state_t::pending);
	BOOST_ASSERT(connector->orders_log.back()->linked_trade_id.empty());

	BOOST_ASSERT(connector->trades_log.size() == 0);

	// ACT
	trader.close_trade(trade_id, -1L);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->orders_log.back()->current_state == tbp::order::state_t::canceled);
	BOOST_ASSERT(connector->trades_log.size() == 0);

	// ACT
	trader.close_trade(trade_id, -1L);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 1);
	BOOST_ASSERT(connector->orders_log.back()->current_state == tbp::order::state_t::canceled);
	BOOST_ASSERT(connector->trades_log.size() == 0);
}

BOOST_FIXTURE_TEST_CASE(trader_close_not_opened_trade, common_fixture)
{
	// INIT
	temp_folder working_dir;
	mock_connector::ptr connector = std::make_shared<mock_connector>();
	connector->fill_order_after_creation = true;
	tbp::oanda::trader trader(connector, working_dir.path);
	const auto trade_id = unique_string();

	BOOST_ASSERT(connector->orders_log.size() == 0);
	BOOST_ASSERT(connector->trades_log.size() == 0);

	// ACT / ASSERT
	BOOST_ASSERT_EXCEPT(trader.close_trade(trade_id, -1L), std::runtime_error);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 0);
	BOOST_ASSERT(connector->trades_log.size() == 0);
}

BOOST_FIXTURE_TEST_CASE(trader_close_all_pending_trades, common_fixture)
{
	// INIT
	temp_folder working_dir;
	mock_connector::ptr connector = std::make_shared<mock_connector>();
	connector->fill_order_after_creation = true;
	tbp::oanda::trader trader(connector, working_dir.path);

	BOOST_ASSERT(connector->orders_log.size() == 0);
	BOOST_ASSERT(connector->trades_log.size() == 0);

	// ACT
	trader.open_trade(L"EUR_USD", 2000);

	connector->fill_order_after_creation = false;
	trader.open_trade(L"EUR_USD", 2000);

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 2);
	BOOST_ASSERT(connector->trades_log.size() == 1);

	BOOST_ASSERT(connector->orders_log[0]->state() == tbp::order::state_t::filled);
	BOOST_ASSERT(connector->orders_log[1]->state() == tbp::order::state_t::pending);

	BOOST_ASSERT(connector->trades_log.back()->state() == tbp::trade::state_t::opened);

	// ACT
	trader.close_pending_trades();

	// ASSERT
	BOOST_ASSERT(connector->orders_log.size() == 2);
	BOOST_ASSERT(connector->trades_log.size() == 1);

	BOOST_ASSERT(connector->orders_log[0]->state() == tbp::order::state_t::filled);
	BOOST_ASSERT(connector->orders_log[1]->state() == tbp::order::state_t::canceled);

	BOOST_ASSERT(connector->trades_log.back()->state() == tbp::trade::state_t::closed);
}

BOOST_FIXTURE_TEST_CASE(trader_throws_on_cancelled_trade, common_fixture)
{
	// INIT
	temp_folder working_dir;
	mock_connector::ptr connector = std::make_shared<mock_connector>();
	connector->fill_order_after_creation = false;
	connector->cancel_order_after_creation = true;
	tbp::oanda::trader trader(connector, working_dir.path);

	// ACT / ASSERT
	BOOST_ASSERT_EXCEPT(trader.open_trade(L"EUR_USD", 2000), tbp::trader::trade_canceled);
}