#ifndef AL_BITS_H
#define AL_BITS_H

#include <concepts>

#include "engine/types.h"

namespace al
{
    template<std::unsigned_integral T>
    uSize count_bits(T value)
    {
        uSize result = 0;
        for (uSize it = 0; it < sizeof(T) * 8; it++)
            if (value & (T(1) << it)) result += 1;
        return result;
    };
}

#endif
