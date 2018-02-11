#include <boost/test/unit_test.hpp>

#include <core/analysis.h>

#include <test_helpers/base_fixture.h>

#include <deque>

namespace
{
	struct common_fixture : test_helpers::base_fixture
	{
		const std::deque<double> prices{ 5.3, 6.7, 7.9, 7.1, 5.2, 4.1, 3.5, 5.4, 7.3, 9.4, 8.0, 6.6, 7.9, 9.2, 7.6 };
		const std::deque<double> ethalon_ema{ 0.0, 0.0, 0.0, 6.8, 6.1, 5.3,	4.6, 4.9, 5.9, 7.3, 7.6, 7.2, 7.5, 8.2, 7.9 };

	public:
		template<typename T>
		T round_to(T val, unsigned long precision)
		{
			// SB: round to 10th
			val *= precision;
			val = std::round(val);
			val /= precision;

			return val;
		}

		template<typename container1_t, typename container2_t>
		bool is_equal(const container1_t& lhs, const container2_t& rhs)
		{
			if (lhs.size() != rhs.size())
			{
				return false;
			}

			auto lhs_it = lhs.begin();
			auto rhs_it = rhs.begin();
			for (; lhs_it != lhs.end(); ++lhs_it, ++rhs_it)
			{
				auto lhs_val = round_to(*lhs_it, 10);
				auto rhs_val = round_to(*rhs_it, 10);

				if (lhs_val != rhs_val)
				{
					return false;
				}
			}

			return true;
		}

	public:
		common_fixture()
			: base_fixture(L"analysis")
		{
		}
	};
}

BOOST_FIXTURE_TEST_CASE(calulate_ema, common_fixture)
{
	// INIT
	std::deque<double> ema;

	// ACT
	tbp::analysis::calculate_ema(prices, &ema, 4);

	// ASSERT
	BOOST_ASSERT(is_equal(ema, ethalon_ema));
}

BOOST_FIXTURE_TEST_CASE(ema_length_valid_limits, common_fixture)
{
	// ACT
	for (unsigned long i = 1; i < 16; ++i)
	{
		std::deque<double> ema;
		tbp::analysis::calculate_ema(prices, &ema, i);

		for (auto j = 0UL; j < (i - 1UL); ++j)
		{
			BOOST_ASSERT(0.0 == ema[j]);
		}
	}
}

BOOST_FIXTURE_TEST_CASE(ema_length_invalid_limits, common_fixture)
{
	{
		// ACT / ASSERT
		std::deque<double> ema;
		BOOST_ASSERT_EXCEPT(tbp::analysis::calculate_ema(prices, &ema, 0), std::runtime_error);
	}

	{
		// ACT / ASSERT
		std::deque<double> ema;
		BOOST_ASSERT_EXCEPT(tbp::analysis::calculate_ema(prices, &ema, boost::numeric_cast<unsigned long>(prices.size() + 1)), std::runtime_error);
	}
}

BOOST_FIXTURE_TEST_CASE(ema_throws_on_invalid_input, common_fixture)
{
	// INIT
	std::deque<double> ema = prices;

	// ACT / ASSERT
	BOOST_ASSERT_EXCEPT(tbp::analysis::calculate_ema(prices, &ema, 4), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(partial_ema_calculation, common_fixture)
{
	for (size_t i = 0; i < ethalon_ema.size(); ++i)
	{
		std::deque<double> ema(ethalon_ema.begin(), ethalon_ema.begin() + i);

		// ACT
		tbp::analysis::calculate_ema(prices, &ema, 4);

		// ASSERT
		BOOST_ASSERT(is_equal(ema, ethalon_ema));
	}
}

BOOST_FIXTURE_TEST_CASE(ema_calculation_for_different_containers, common_fixture)
{
	// std::deque
	{
		// INIT
		std::deque<double> ema;

		// ACT
		tbp::analysis::calculate_ema(prices, &ema, 4);

		// ASSERT
		BOOST_ASSERT(is_equal(ema, ethalon_ema));
	}

	// std::vector
	{
		// INIT
		const std::vector<double> prices(prices.begin(), prices.end());
		const std::vector<double> ethalon_ema(ethalon_ema.begin(), ethalon_ema.end());
		std::vector<double> ema;

		// ACT
		tbp::analysis::calculate_ema(prices, &ema, 4);

		// ASSERT
		BOOST_ASSERT(is_equal(ema, ethalon_ema));
	}

	// std::list
	{
		// INIT
		const std::vector<double> prices(prices.begin(), prices.end());
		const std::vector<double> ethalon_ema(ethalon_ema.begin(), ethalon_ema.end());
		std::list<double> ema;

		// ACT
		tbp::analysis::calculate_ema(prices, &ema, 4);

		// ASSERT
		BOOST_ASSERT(is_equal(ema, ethalon_ema));
	}

	// mix
	{
		// INIT
		const std::vector<double> prices(prices.begin(), prices.end());
		std::deque<double> ema;

		// ACT
		tbp::analysis::calculate_ema(prices, &ema, 4);

		// ASSERT
		BOOST_ASSERT(is_equal(ema, ethalon_ema));
	}

	// mix
	{
		// INIT
		const std::vector<double> ethalon_ema(ethalon_ema.begin(), ethalon_ema.end());
		std::vector<double> ema;

		// ACT
		tbp::analysis::calculate_ema(prices, &ema, 4);

		// ASSERT
		BOOST_ASSERT(is_equal(ema, ethalon_ema));
	}

	// mix
	{
		// INIT
		const std::vector<double> ethalon_ema(ethalon_ema.begin(), ethalon_ema.end());
		std::list<double> ema;

		// ACT
		tbp::analysis::calculate_ema(prices, &ema, 4);

		// ASSERT
		BOOST_ASSERT(is_equal(ema, ethalon_ema));
	}
}