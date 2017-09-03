#pragma once

#include <win/exception.h>

#include <windows.h>

namespace win
{
	template<typename traits_t>
	class handle : traits_t, sb::noncopyable
	{
		typename traits_t::value_type m_value;

	public:
		typename const traits_t::value_type& value;

	public:
		bool empty() const
		{
			return traits_t::empty_value == m_value;
		}

	public:
		handle(handle&& rhs)
			: m_value(rhs.value)
			, value(m_value)
		{
			rhs.value = traits_t::empty_value;
		}

		handle(typename traits_t::value_type val)
			: m_value(val)
			, value(m_value)
		{
		}

		~handle()
		{
			if (empty())
			{
				return;
			}

			traits_t::close_handle(m_value);
			m_value = traits_t::empty_value;
		}
	};

	///////////////////////////////////////////////////////////////////////////////
	// traits

	struct common_traits
	{
		using value_type = HANDLE;
		
		const value_type empty_value = nullptr;
		
		void close_handle(const value_type& val)
		{
			if (0 == ::CloseHandle(val))
			{
				throw win::exception(L"CloseHandle call failed!");
			}
		}
	};

	///////////////////////////////////////////////////////////////////////////////
	// handle types

	using common_handle = handle<common_traits>;
}