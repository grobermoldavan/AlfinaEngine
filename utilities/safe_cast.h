#ifndef AL_SAFE_CAST_H
#define AL_SAFE_CAST_H

#ifndef AL_SAFE_CAST_ASSERT
#   include <cassert>
#   define AL_SAFE_CAST_ASSERT(cond) assert(cond)
#endif

#include <cstdint>

namespace al
{
    int64_t safe_cast_uint64_to_int64(uint64_t value)
    {
        AL_SAFE_CAST_ASSERT(value <= INT64_MAX);
        return static_cast<int64_t>(value);
    }

    uint32_t safe_cast_int64_to_uint32(int64_t value)
    {
        AL_SAFE_CAST_ASSERT(value >= 0);
        AL_SAFE_CAST_ASSERT(value <= UINT32_MAX);
        return static_cast<uint32_t>(value);
    }

    uint32_t safe_cast_uint64_to_uint32(uint64_t value)
    {
        AL_SAFE_CAST_ASSERT(value <= UINT32_MAX);
        return static_cast<uint32_t>(value);
    }
}

#undef AL_SAFE_CAST_ASSERT

#endif
