#ifndef AL_DEFER_H
#define AL_DEFER_H

#include "engine/utilities/function.h"

// This macro combo generates a unique name 
// based on line of file where it is being used

#ifndef __UNIQUE_NAME
#   define __PP_CAT(a, b) __PP_CAT_I(a, b)
#   define __PP_CAT_I(a, b) __PP_CAT_II(~, a ## b)
#   define __PP_CAT_II(p, res) res
#   define __UNIQUE_NAME(base) __PP_CAT(base, __LINE__)
#endif

#define defer(...) ::al::Defer __UNIQUE_NAME(deferObj)([&]() { __VA_ARGS__; })

namespace al
{
    class Defer
    {
    public:
        Defer(const Function<void(void)>& func)
            : function{ func }
        { }

        ~Defer()
        {
            function();
        }

    private:
        Function<void(void)> function;
    };
}

#endif
