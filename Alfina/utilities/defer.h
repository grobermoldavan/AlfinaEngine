#ifndef __ALFINA_DEFER_H__
#define __ALFINA_DEFER_H__

#include <functional>

// This macro combo generates a unique name 
// based on line of file where it is being used

#define __PP_CAT(a, b) __PP_CAT_I(a, b)
#define __PP_CAT_I(a, b) __PP_CAT_II(~, a ## b)
#define __PP_CAT_II(p, res) res
#define __UNIQUE_NAME(base) __PP_CAT(base, __LINE__)

#define defer(func) al::Defer __UNIQUE_NAME(deferObj)([&]() func)

namespace al
{
	class Defer
	{
	public:
		Defer(std::function<void(void)>&& func)
			: function(std::move(func))
		{ }

		~Defer()
		{
			function();
		}

		// @TODO : delete other operators
		Defer& operator = (const Defer& defer) = delete;

	private:
		std::function<void(void)> function;
	};
}

#endif