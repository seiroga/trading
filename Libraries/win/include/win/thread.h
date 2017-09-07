#pragma once

#include <common/constrains.h>
#include <win/handle.h>

#include <vector>
#include <string>

namespace win
{
	struct waitable
	{
		virtual bool wait(unsigned long timeout) = 0;
	};

	struct lockable
	{
		virtual bool try_lock() = 0;
		virtual void lock() = 0;
		virtual void unlock() = 0;
	};

	class scoped_lock : sb::noncopyable
	{
		lockable* m_lock;

	public:
		scoped_lock(lockable& lock)
			: m_lock(&lock)
		{
			m_lock->lock();
		}

		scoped_lock(scoped_lock&& rhs)
			: m_lock(rhs.m_lock)
		{
			rhs.m_lock = nullptr;
		}

		~scoped_lock()
		{
			if (nullptr != m_lock)
			{
				m_lock->unlock();
			}
		}
	};

	template<typename T, typename ...args_t>
	void get_args_impl(std::vector<HANDLE>& result, const T& arg, const args_t&... args)
	{
		result.push_back(arg.value);
		get_args_impl(result, args...);
	}

	template<typename T>
	void get_args_impl(std::vector<HANDLE>& result, const T& arg)
	{
		result.push_back(arg.value);
	}

	template<typename T, typename ...args_t>
	std::vector<HANDLE> get_args(const T& arg, const args_t&... args)
	{
		std::vector<HANDLE> res;
		get_args_impl(res, arg, args...);

		return res;
	}

	template<typename ...args_t>
	std::pair<bool, unsigned long> wait_for_multiple_objects(bool wait_all, unsigned long timeout, const args_t&... args)
	{
		auto args_vec = get_args(args...);
		auto res = ::WaitForMultipleObjects((unsigned long)args_vec.size(), args_vec.data(), wait_all, timeout);
		switch (res)
		{
			case WAIT_FAILED:
			{
				throw win::exception(L"WaitForMultipleObjects call failed!");
			}

			case WAIT_TIMEOUT:
			{
				return { true, -1 };
			}
		}

		if (res >= WAIT_OBJECT_0 && res < WAIT_ABANDONED_0)
		{
			return { false, res - WAIT_OBJECT_0 };
		}
		else if (res >= WAIT_ABANDONED_0)
		{
			return { false, res - WAIT_ABANDONED_0 };
		}

		throw win::exception(L"WaitForMultipleObjects call failed! Unknown result value!");
	}

	/////////////////////////////////////////////////////////////////////////////////
	// event

	class event : public common_handle, public waitable
	{
	public:
		virtual bool wait(unsigned long timeout) override;

	public:
		void set();
		void reset();

	public:
		event(const std::wstring& name, bool manual_reset, bool initial_state);
		event(bool manual_reset, bool initial_state);
	};
}