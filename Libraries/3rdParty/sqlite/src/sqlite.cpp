#include <sqlite/sqlite.h>
#include "sqlite3.h"

namespace sqlite
{
	namespace
	{
		std::wstring get_error_message(sqlite3* handle)
		{
			return static_cast<const wchar_t*>(::sqlite3_errmsg16(handle));
		}

		sqlite3* open_db(const std::wstring& path)
		{
			sqlite3* handle = nullptr;
			sqlite::exception::check(handle, ::sqlite3_open16(path.c_str(), &handle));

			return handle;
		}

		sqlite3_stmt* prepare_statement(sqlite3* db_handle, const std::wstring& query)
		{
			sqlite3_stmt* result = nullptr;
			exception::check(db_handle, ::sqlite3_prepare16_v2(db_handle, query.c_str(), (int)query.length() * sizeof(wchar_t), &result, nullptr));

			return result;
		}
	}

	//////////////////////////////////////////////////////////
	// exception impl

	void exception::check(sqlite3* handle, int code)
	{
		if (SQLITE_OK == code)
			return;

		throw exception(code, get_error_message(handle));
	}

	//////////////////////////////////////////////////////////
	// statement impl

	void statement::reset()
	{
		// reset statement
		exception::check(m_db_handle, ::sqlite3_reset(m_handle));
	}

	bool statement::step()
	{
		// execute
		auto res_code = ::sqlite3_step(m_handle);
		switch (res_code)
		{
			case SQLITE_DONE:
				return false;
			case SQLITE_ROW:
				return true;

		default:
			exception::check(m_db_handle, res_code);
		}

		return false;
	}

	value_t statement::get_value_impl(int column_index)
	{
		auto value_type = ::sqlite3_column_type(m_handle, column_index);
		value_t res;
		switch (value_type)
		{
		case SQLITE_INTEGER:
			{
				res = ::sqlite3_column_int64(m_handle, column_index);
			}
			break;

		case SQLITE_FLOAT:
			{
				res = ::sqlite3_column_double(m_handle, column_index);
			}
			break;

		case SQLITE_TEXT:
			{
				auto pstr = static_cast<const wchar_t*>(::sqlite3_column_text16(m_handle, column_index));
				res = std::wstring(pstr);
			}
			break;

		case SQLITE_BLOB:
			{
				auto size = ::sqlite3_column_bytes(m_handle, column_index);
				auto data = static_cast<const byte_t*>(::sqlite3_column_blob(m_handle, column_index));

				res = binary_t(data, data + size);
			}
			break;

		case SQLITE_NULL:
			{
				res = vnull;
			}
			break;

		default:
			throw std::runtime_error(__FUNCTION__ " Invalid DB type!");
		}

		return res;
	}

	void statement::bind_value(const value_t& val, int index)
	{
		class value_binder : public boost::static_visitor<void>
		{
			sqlite3* const db_handle;
			sqlite3_stmt* const stmt_handle;
			const int index;

		public:
			void operator ()(int val)
			{
				exception::check(db_handle, ::sqlite3_bind_int(stmt_handle, index, val));
			}

			void operator ()(__int64 val)
			{
				exception::check(db_handle, ::sqlite3_bind_int64(stmt_handle, index, val));
			}

			void operator ()(double val)
			{
				exception::check(db_handle, ::sqlite3_bind_double(stmt_handle, index, val));
			}

			void operator ()(const binary_t& val)
			{
				// SB: can be performance overhead here, since SQLITE makes own copy of data
				exception::check(db_handle, ::sqlite3_bind_blob(stmt_handle, index, val.data(), (int)val.size(), SQLITE_TRANSIENT));
			}

			void operator ()(const std::wstring& val)
			{
				// SB: can be performance overhead here, since SQLITE makes own copy of data
				exception::check(db_handle, ::sqlite3_bind_text16(stmt_handle, index, val.data(), (int)val.size() * sizeof(wchar_t), SQLITE_TRANSIENT));
			}

			void operator ()(const vnull_t& val)
			{
				exception::check(db_handle, ::sqlite3_bind_zeroblob(stmt_handle, index, -1));
			}

		public:
			value_binder(sqlite3* db_handle, sqlite3_stmt* stmt_handle, int index)
				: db_handle(db_handle)
				, stmt_handle(stmt_handle)
				, index(index)
			{
			}
		};

		val.apply_visitor(value_binder(m_db_handle, m_handle, index));
	}

	statement::statement(sqlite3* db_handle, const std::wstring& query)
		: m_db_handle(db_handle)
		, m_handle(prepare_statement(db_handle, query))
	{
	}

	statement::~statement()
	{
		// TODO: add result validation logic
		::sqlite3_finalize(m_handle);
		m_handle = nullptr;
	}

	//////////////////////////////////////////////////////////
	// connection impl

	statement::ptr connection::create_statement(const std::wstring& query)
	{
		return std::make_shared<statement>(m_handle, query);
	}

	connection::ptr connection::create(const std::wstring& db_path)
	{
		return std::make_shared<connection>(db_path);
	}

	connection::connection(const std::wstring& db_path)
		: m_handle(open_db(db_path))
	{
	}

	connection::~connection()
	{
		// TODO: add result validation logic
		::sqlite3_close(m_handle);
		m_handle = nullptr;
	}
}