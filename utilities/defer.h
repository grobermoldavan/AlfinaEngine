#ifndef AL_DEFER_H
#define AL_DEFER_H

#include "function.h"

// This macro combo generates a unique name 
// based on line of file where it is being used

#ifndef __UNIQUE_NAME
#   define __PP_CAT(a, b) __PP_CAT_I(a, b)
#   define __PP_CAT_I(a, b) __PP_CAT_II(~, a ## b)
#   define __PP_CAT_II(p, res) res
#   define __UNIQUE_NAME(base) __PP_CAT(base, __LINE__)
#endif

#define defer(func) al::Defer __UNIQUE_NAME(deferObj)([&]() func)

namespace al
{
	class Defer
	{
	public:
		Defer(al::Function<void(void)>&& func)
			: function(std::move(func))
		{ }

		~Defer()
		{
			function();
		}

		// @TODO : delete other operators
		Defer& operator = (const Defer& defer) = delete;

	private:
		al::Function<void(void)> function;
	};
}

#endif