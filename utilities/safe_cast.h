#ifndef AL_SAFE_CAST_H
#define AL_SAFE_CAST_H

#include <cstdint>

namespace al
{
    bool safe_cast_uint64_to_int64(uint64_t value, int64_t* result)
    {
        if (value > INT64_MAX)
        {
            return false;
        }
        *result = static_cast<int64_t>(value);
        return true;
    }

    bool safe_cast_int64_to_uint32(int64_t value, uint32_t* result)
    {
        if (value < 0)
        {
            return false;
        }
        if (value > UINT32_MAX)
        {
            return false;
        }
        *result = static_cast<uint32_t>(value);
        return true;
    }

    bool safe_cast_uint64_to_uint32(uint64_t value, uint32_t* result)
    {
        if (value > UINT32_MAX)
        {
            return false;
        }
        *result = static_cast<uint32_t>(value);
        return true;
    }
}

#endif
