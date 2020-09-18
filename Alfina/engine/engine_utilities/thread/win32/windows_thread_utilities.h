#ifndef __ALFINA_WINDOWS_THREAD_UTILITIES_H__
#define __ALFINA_WINDOWS_THREAD_UTILITIES_H__

#include <Windows.h>

#include "../thread_utilities.h"

#include "engine/engine_utilities/logging/logging.h"

namespace al::engine
{
	static int convert_priority(ThreadPriority priority)
	{
		switch (priority)
		{
		case ThreadPriority::LOWEST:		return THREAD_PRIORITY_LOWEST;
		case ThreadPriority::BELOW_NORMAL:	return THREAD_PRIORITY_BELOW_NORMAL;
		case ThreadPriority::NORMAL:		return THREAD_PRIORITY_NORMAL;
		case ThreadPriority::ABOVE_NORMAL:	return THREAD_PRIORITY_ABOVE_NORMAL;
		case ThreadPriority::HIGHEST:		return THREAD_PRIORITY_HIGHEST;
		}
		AL_LOG(Logger::Type::ERROR_MSG, "Unknown ThreadPriority value : ", static_cast<int>(priority))
		return 0;
	}

	void set_thread_priority(std::thread& thread, ThreadPriority priority)
	{
		HANDLE threadHandle = thread.native_handle();
		BOOL result = ::SetThreadPriority(threadHandle, convert_priority(priority));
		AL_ASSERT(result)
	}
}

#endif
