#ifndef AL_FLAGS_H
#define AL_FLAGS_H

#include <cstdint>

namespace al
{
    struct Flags32
    {
        Flags32(uint32_t initial_flags = 0) noexcept
            : flags{ initial_flags }
        { }

        inline void set_flag(uint32_t flag) noexcept
        {
            flags |= 1 << flag;
        }

        inline bool get_flag(uint32_t flag) const noexcept
        {
            return static_cast<bool>(flags & (1 << flag));
        }

        inline void clear_flag(uint32_t flag) noexcept
        {
            flags &= ~(1 << flag);
        }

        inline void clear() noexcept
        {
            flags = 0;
        }

        uint32_t flags;
    };

    struct Flags128
    {
        Flags128(uint64_t f1 = 0, uint64_t f2 = 0) noexcept
            : flags{ f1, f2 }
        { }

        inline void set_flag(uint64_t flag) noexcept
        {
            const uint64_t id = flag / 64ULL;
            flags[id] |= 1ULL << (flag - (id * 64ULL));
        }

        inline bool get_flag(uint64_t flag) const noexcept
        {
            const uint64_t id = flag / 64ULL;
            return static_cast<bool>(flags[id] & (1ULL << (flag - (id * 64ULL))));
        }

        inline void clear_flag(uint64_t flag) noexcept
        {
            const uint64_t id = flag / 64ULL;
            flags[id] &= ~(1ULL << (flag - (id * 64ULL)));
        }

        inline void clear() noexcept
        {
            flags[0] = flags[1] = 0;
        }

        bool operator == (const Flags128& other) const noexcept
        {
            return (flags[0] == other.flags[0]) && (flags[1] == other.flags[1]);
        }

        uint64_t flags[2];
    };
}

#endif
