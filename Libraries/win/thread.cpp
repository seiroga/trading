#include <win/thread.h>

namespace win
{
	HANDLE get_value(void* handle)
	{
		return handle;
	}

	///////////////////////////////////////////////////////////////////////
	// event implementation

	bool event::wait(unsigned long timeout)
	{
		return WAIT_OBJECT_0 == ::WaitForSingleObject(value, timeout);
	}

	void event::set()
	{
		if (0 == ::SetEvent(value))
		{
			throw win::exception(L"SetEvent call failed!");
		}
	}

	void event::reset()
	{
		if (0 == ::ResetEvent(value))
		{
			throw win::exception(L"SetEvent call failed!");
		}
	}

	event::event(const std::wstring& name, bool manual_reset, bool initial_state)
		: common_handle(::CreateEventW(nullptr, manual_reset ? TRUE : FALSE, initial_state ? TRUE : FALSE, name.c_str()))
	{
		if (empty())
		{
			throw win::exception(L"CreateEvent call failed!");
		}
	}

	event::event(bool manual_reset, bool initial_state)
		: common_handle(::CreateEventW(nullptr, manual_reset ? TRUE : FALSE, initial_state ? TRUE : FALSE, nullptr))
	{
		if (empty())
		{
			throw win::exception(L"CreateEvent call failed!");
		}
	}

	/////////////////////////////////////////////////////////////////////////////////
	// crirical_section

	bool critical_section::try_lock()
	{
		return 0 != ::TryEnterCriticalSection(&m_cs);
	}

	void critical_section::lock()
	{
		::EnterCriticalSection(&m_cs);
	}

	void critical_section::unlock()
	{
		::LeaveCriticalSection(&m_cs);
	}

	critical_section::critical_section()
	{
		::InitializeCriticalSection(&m_cs);
	}

	critical_section::~critical_section()
	{
		::DeleteCriticalSection(&m_cs);
	}

	///////////////////////////////////////////////////////////////////////////////////
	// mrsw_lock implementation

	bool mrsw_lock::try_lock(acquire_mode_t mode)
	{
		auto acquire_func_ptr = acquire_mode_t::exlusive == mode ? &::TryAcquireSRWLockExclusive : &::TryAcquireSRWLockShared;
		return 0 == acquire_func_ptr(&m_handle);
	}

	void mrsw_lock::lock(acquire_mode_t mode)
	{
		auto acquire_func_ptr = acquire_mode_t::exlusive == mode ? &::AcquireSRWLockExclusive : &::AcquireSRWLockShared;
		acquire_func_ptr(&m_handle);
	}

	void mrsw_lock::unlock(acquire_mode_t mode)
	{
		auto acquire_func_ptr = acquire_mode_t::exlusive == mode ? &::ReleaseSRWLockExclusive : &::ReleaseSRWLockShared;
		acquire_func_ptr(&m_handle);
	}

	mrsw_lock::mrsw_lock()
		: m_handle(SRWLOCK_INIT)
	{
	}

	mrsw_lock::~mrsw_lock()
	{
	}
}