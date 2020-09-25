#ifndef __ALFINA_THREAD_UTILITIES_H__
#define __ALFINA_THREAD_UTILITIES_H__

#include <thread>

namespace al::engine
{
	enum class ThreadPriority
	{
		LOWEST,
		BELOW_NORMAL,
		NORMAL,
		ABOVE_NORMAL,
		HIGHEST
	};

	// must be implemented in platform side
	void set_thread_priority		(std::thread& thread, ThreadPriority priority);
	void set_current_thread_priority(ThreadPriority priority);
}

#endif
