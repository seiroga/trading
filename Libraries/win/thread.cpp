#include <win/thread.h>

namespace win
{
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
}