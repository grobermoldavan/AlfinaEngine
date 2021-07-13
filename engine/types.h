#ifndef AL_TYPES_H
#define AL_TYPES_H

#include <cstddef>
#include <cstdint>

#include <limits>

namespace al
{
    using u8    = uint8_t;
    using u16   = uint16_t;
    using u32   = uint32_t;
    using u64   = uint64_t;

    using s8    = int8_t;
    using s16   = int16_t;
    using s32   = int32_t;
    using s64   = int64_t;

    using f32   = float;
    using f64   = double;

    using uSize = std::size_t;
    using uPtr  = uintptr_t;

    template<typename T, typename U = T> constexpr U max_value() { return U(std::numeric_limits<T>::max()); }
    template<typename T, typename U = T> constexpr U min_value() { return U(std::numeric_limits<T>::min()); }
    template<typename T, typename U = T> constexpr U is_signed() { return U(std::numeric_limits<T>::is_signed); }
    template<typename T, typename U = T> constexpr U is_integer() { return U(std::numeric_limits<T>::is_integer); }
}

#endif
