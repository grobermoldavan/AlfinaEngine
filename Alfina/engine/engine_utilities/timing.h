#ifndef __ALFINA_TIMING_H__
#define __ALFINA_TIMING_H__

#include <functional>
#include <chrono>
#include <type_traits>

namespace al::engine
{
	class ScopeTimer
	{
	public:
		using Callback = std::function<void(double)>;
		using TimeType = std::invoke_result<decltype(&std::chrono::high_resolution_clock::now)>::type;

		ScopeTimer(Callback cb)
			: finishCb	{ std::move(cb) }
			, startTime	{ std::chrono::high_resolution_clock::now() }
		{ }

		~ScopeTimer()
		{
			TimeType endTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double, std::milli> result = endTime - startTime;
			finishCb(result.count());
		}

	private:
		Callback finishCb;
		TimeType startTime;
	};
}

#endif
