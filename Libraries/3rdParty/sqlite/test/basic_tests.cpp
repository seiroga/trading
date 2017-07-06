#define BOOST_TEST_MODULE SQLITE Tests

#include <boost/test/included/unit_test.hpp>

#include <sqlite/sqlite.h>

#include <objbase.h>
#include <comdef.h>

#include <shlobj.h>

namespace win
{
	struct exception 
	{
		const DWORD code;
		const std::wstring msg;

		exception(const std::wstring& msg, DWORD err_code = ::GetLastError())
			: code(err_code)
			, msg(msg)
		{
		}
	};
}

namespace
{
	bool operator==(const std::wstring& lhs, const std::wstring& rhs)
	{
		return std::equal(lhs.begin(), lhs.end(), rhs.begin());
	}

	std::wstring generate_guid()
	{
		GUID guid = { 0 };
		if (S_OK != ::CoCreateGuid(&guid))
		{
			throw win::exception(L"CoCreateGuid call failed!");
		}

		auto co_task_mem_free = [](LPOLESTR ptr)
		{
			::CoTaskMemFree(ptr);
		};
		
		LPOLESTR guid_str = nullptr;
		if (S_OK != ::StringFromCLSID(guid, &guid_str))
		{
			throw win::exception(L"StringFromCLSID call failed!");
		}

		std::unique_ptr<OLECHAR, decltype(co_task_mem_free)> guid_ptr(guid_str, co_task_mem_free);
		std::wstring result(guid_str);

		return result;
	}

	std::wstring get_module_directory()
	{
		wchar_t path_buff[1024] = { 0 };
		if (0 == ::GetModuleFileNameW(nullptr, path_buff, _countof(path_buff)))
		{
			throw win::exception(L"GetModuleFileNameW call failed!");
		}

		std::wstring path(path_buff);
		size_t pos = path.find_last_of(L"\\");
		if (std::wstring::npos == pos)
		{
			throw std::runtime_error( __FUNCTION__ " Invalid path format!");
		}

		return path.substr(0, pos);
	}
}

struct basic_fixture
{
	struct temp_folder
	{
		const std::wstring path;

		temp_folder(const std::wstring& path = get_module_directory() + L"\\test_data\\" + unique_string())
			: path(path)
		{
			int result = ::SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
			if (ERROR_SUCCESS != result && ERROR_FILE_EXISTS != result && ERROR_ALREADY_EXISTS != result)
				throw win::exception(L"Failed to create path: " + path, result);
		}

		~temp_folder()
		{
			std::vector<wchar_t> path_with_double_terminator(path.length() + 2, L'\0');
			std::copy(path.begin(), path.end(), path_with_double_terminator.begin());
			SHFILEOPSTRUCTW shf_info = { 0 };
			shf_info.pFrom = path_with_double_terminator.data();
			shf_info.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NO_UI | FOF_SILENT;
			shf_info.wFunc = FO_DELETE;
			int res = ::SHFileOperationW(&shf_info);
		}
	};

	static std::wstring unique_string()
	{
		return generate_guid();
	}
};

struct temp_dir_fixture : basic_fixture
{
	temp_folder test_data_root;
	
	temp_dir_fixture()
		: test_data_root(get_module_directory() + L"\\test_data")
	{
	}
};

BOOST_FIXTURE_TEST_CASE(create_new, temp_dir_fixture)
{
	// INIT
	temp_dir_fixture fixture;
	basic_fixture::temp_folder tmp_folder;
	const auto db_name = basic_fixture::unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(db != nullptr);
}

BOOST_FIXTURE_TEST_CASE(open_existing, temp_dir_fixture)
{
	// INIT
	basic_fixture::temp_folder tmp_folder;
	const auto db_name = basic_fixture::unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(db != nullptr);

	// INIT
	db = nullptr;

	// ACT
	db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(db != nullptr);
}

BOOST_FIXTURE_TEST_CASE(create_table, temp_dir_fixture)
{
	// INIT
	basic_fixture::temp_folder tmp_folder;
	const auto db_name = basic_fixture::unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(db != nullptr);

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

BOOST_FIXTURE_TEST_CASE(insert_data, temp_dir_fixture)
{
	// INIT
	basic_fixture::temp_folder tmp_folder;
	const auto db_name = basic_fixture::unique_string();
	auto db = sqlite::connection::create(tmp_folder.path + L"\\" + db_name);

	// ASSERT
	BOOST_TEST(db != nullptr);

	// INIT
	const std::wstring expected_value = L"some text";
	auto st = db->create_statement(L"CREATE TABLE T1(column1 INTEGER, column2 TEXT, column3 FLOAT, PRIMARY KEY(column1))");
	st->step();

	// ACT
	st = db->create_statement(L"INSERT INTO T1 VALUES(?1, ?2, ?3)");
	st->bind_value(24, 1);
	st->bind_value(expected_value, 2);
	st->bind_value(45345.0342, 3);
	st->step();

	st = db->create_statement(L"SELECT column2 FROM T1 WHERE column1 = ?1");
	st->bind_value(24, 1);
	
	// ACT / ASSERT
	BOOST_TEST(st->step());

	auto text_value = st->get_value<std::wstring>(0);
	bool res = text_value == expected_value;
	BOOST_TEST(res);
}