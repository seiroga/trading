#include <boost/test/unit_test.hpp>

#include <sqlite/sqlite.h>

#include <test_helpers/base_fixture.h>

#include <objbase.h>
#include <comdef.h>

#include <shlobj.h>

namespace
{
	bool operator==(const std::wstring& lhs, const std::wstring& rhs)
	{
		return std::equal(lhs.begin(), lhs.end(), rhs.begin());
	}
}

BOOST_FIXTURE_TEST_CASE(create_new, test_helpers::temp_dir_fixture)
{
	// INIT
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(nullptr != db);
}

BOOST_FIXTURE_TEST_CASE(open_existing, test_helpers::temp_dir_fixture)
{
	// INIT
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(nullptr != db);

	// INIT
	db = nullptr;

	// ACT
	db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(nullptr != db);
}

BOOST_FIXTURE_TEST_CASE(create_table, test_helpers::temp_dir_fixture)
{
	// INIT
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(nullptr != db);

	// ACT
	auto st = db->create_statement(L"CREATE TABLE T1(column1 INTEGER, column2 TEXT, column3 FLOAT, PRIMARY KEY(column1))");
	st->step();

	bool thrown = false;
	try
	{
		// try to create table with same name
		st = db->create_statement(L"CREATE TABLE T1(primary_column INTEGER, PRIMARY KEY(primary_column))");
		st->step();
	}
	catch (const sqlite::exception& )
	{
		thrown = true;
	}

	BOOST_TEST(thrown);
}

BOOST_FIXTURE_TEST_CASE(insert_data, test_helpers::temp_dir_fixture)
{
	// INIT
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(nullptr != db);

	// INIT
	const std::wstring expected_value = L"some text";
	const sqlite::binary_t bin_data(20, 0xff);
	auto st = db->create_statement(L"CREATE TABLE T1(column1 INTEGER, column2 TEXT, column3 FLOAT, column4 BLOB, PRIMARY KEY(column1))");
	st->step();

	// ACT
	st = db->create_statement(L"INSERT INTO T1 VALUES(?1, ?2, ?3, ?4)");
	st->bind_value(24, 1);
	st->bind_value(expected_value, 2);
	st->bind_value(45345.0342, 3);
	st->bind_value(bin_data, 4);
	st->step();

	st = db->create_statement(L"SELECT column2, column4 FROM T1 WHERE column1 = ?1");
	st->bind_value(24, 1);
	
	// ACT / ASSERT
	BOOST_TEST(st->step());

	// ASSERT
	auto text_value = st->get_value<std::wstring>(0);
	bool res = text_value == expected_value;
	BOOST_TEST(res);

	auto bin_value = st->get_value<sqlite::binary_t>(1);
	res = bin_data == bin_value;
	BOOST_TEST(res);
}

BOOST_FIXTURE_TEST_CASE(int_data, test_helpers::temp_dir_fixture)
{
	// INIT
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(nullptr != db);

	// INIT
	const auto expected_value = 6476347;
	const sqlite::binary_t bin_data(20, 0xff);
	auto st = db->create_statement(L"CREATE TABLE T1(column1 INTEGER, column2 INTEGER, PRIMARY KEY(column1))");
	st->step();

	// ACT
	st = db->create_statement(L"INSERT INTO T1 VALUES(?1, ?2)");
	st->bind_value(24, 1);
	st->bind_value(expected_value, 2);
	st->step();

	st = db->create_statement(L"SELECT column2 FROM T1 WHERE column1 = ?1");
	st->bind_value(24, 1);

	// ACT / ASSERT
	BOOST_TEST(st->step());

	// ASSERT
	auto int_value = st->get_value<int>(0);
	BOOST_TEST(int_value == expected_value);

	auto int64_value = st->get_value<__int64>(0);
	BOOST_TEST(int64_value == expected_value);
}

BOOST_FIXTURE_TEST_CASE(transactional, test_helpers::temp_dir_fixture)
{
	// INIT
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(nullptr != db);

	// INIT
	const std::wstring expected_value = L"some text";
	const sqlite::binary_t bin_data(20, 0xff);
	const auto double_value = 45345.0342;

	auto st = db->create_statement(L"CREATE TABLE T1(column1 INTEGER, column2 TEXT, column3 FLOAT, column4 BLOB, PRIMARY KEY(column1))");
	st->step();

	{
		// INIT
		sqlite::transaction t(db);

		// ACT
		st = db->create_statement(L"INSERT INTO T1 VALUES(?1, ?2, ?3, ?4)");
		st->bind_value(24, 1);
		st->bind_value(expected_value, 2);
		st->bind_value(double_value, 3);
		st->bind_value(bin_data, 4);
		st->step();

		t.rollback();

		st = db->create_statement(L"SELECT column2, column4 FROM T1 WHERE column1 = ?1");
		st->bind_value(24, 1);

		// ACT / ASSERT
		BOOST_TEST(!st->step());
	}

	{
		// INIT
		sqlite::transaction t(db);

		// ACT
		st = db->create_statement(L"INSERT INTO T1 VALUES(?1, ?2, ?3, ?4)");
		st->bind_value(24, 1);
		st->bind_value(expected_value, 2);
		st->bind_value(45345.0342, 3);
		st->bind_value(bin_data, 4);
		st->step();

		t.commit();

		st = db->create_statement(L"SELECT column2, column3, column4 FROM T1 WHERE column1 = ?1");
		st->bind_value(24, 1);

		// ACT / ASSERT
		BOOST_TEST(st->step());

		// ASSERT
		auto text_value = st->get_value<std::wstring>(0);
		bool res = text_value == expected_value;
		BOOST_TEST(res);

		auto curr_double_value = st->get_value<double>(1);
		BOOST_TEST(curr_double_value == double_value);

		auto bin_value = st->get_value<sqlite::binary_t>(2);
		res = bin_data == bin_value;
		BOOST_TEST(res);
	}
}

BOOST_FIXTURE_TEST_CASE(synchronous, test_helpers::temp_dir_fixture)
{
	// INIT
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(nullptr != db);

	// ACT
	db->set_synchronous(false);
	db->set_synchronous(true);
}

BOOST_FIXTURE_TEST_CASE(schema_version, test_helpers::temp_dir_fixture)
{
	// INIT
	temp_folder tmp_folder;
	const auto db_name = unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(nullptr != db);

	// ACT
	db->set_schema_version(3);

	// ASSERT
	BOOST_TEST(db->schema_version() == 3);

	// ACT
	db->set_schema_version(1);

	// ASSERT
	BOOST_TEST(db->schema_version() == 1);
}