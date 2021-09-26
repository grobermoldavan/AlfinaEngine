#ifndef AL_ASSERT_H
#define AL_ASSERT_H

#include "engine/types.h"

#ifdef AL_DEBUG
#   define al_assert(cond)                 { if (!(cond)) ::al::assert_impl(#cond, __FILE__, __LINE__, nullptr); }
#   define al_assert_msg(cond, fmt, ...)   { if (!(cond)) ::al::assert_impl(#cond, __FILE__, __LINE__, fmt, __VA_ARGS__); }
#   define al_assert_fail(fmt, ...)        { ::al::assert_impl("fail", __FILE__, __LINE__, fmt, __VA_ARGS__); }
#else
#   define al_assert(cond)
#   define al_assert_msg(cond, fmt, ...)
#   define al_assert_fail(fmt, ...)
#endif

namespace al
{
    template<typename ... Args>
    void assert_impl(const char* condition, const char* file, uSize line, const char* msgFmt, Args ... args);
}

#endif
