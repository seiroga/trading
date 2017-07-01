#pragma once

#include <boost/variant.hpp>

#include <string>
#include <memory>
#include <vector>

struct sqlite3;
struct sqlite3_stmt;

namespace sqlite
{
	struct exception
	{
		int code;
		const std::wstring description;

	public:
		static void check(sqlite3* handle, int code);

	public:
		exception(int code, const std::wstring& desc)
			: code(code)
			, description(desc)
		{
		}
	};

	struct vnull_t {};
	using byte_t = unsigned char;
	using binary_t = std::vector<byte_t>;
	using value_t = boost::variant<int, __int64, double, binary_t, std::wstring, vnull_t>;

	const vnull_t vnull;

	class statement
	{
	public:
		using ptr = std::shared_ptr<statement>;

	private:
		sqlite3* const m_db_handle;
		sqlite3_stmt* m_handle;

	private:
		 value_t get_value_impl(int column_index);

	public:
		void reset();
		void step();

		template<typename T>
		T get_value(int column_index)
		{
			auto value = get_value_impl(column_index);
			auto res = boost::get<T*>(&value);
			if (nullptr == res)
				throw std::runtime_error(__FUNCTION__ " Invalid value type!");

			return *res;
		}

		void bind_value(const value_t& val, int index);

	public:
		statement(sqlite3* db_handle, const std::wstring& query);
		~statement();
	};

	class connection 
	{
	public:
		using ptr = std::shared_ptr<connection>;

	private:
		sqlite3* m_handle;

	private:
		statement::ptr create_statement(const std::wstring& query);

	public:
		connection(const std::wstring& db_path);
		~connection();
	};
}