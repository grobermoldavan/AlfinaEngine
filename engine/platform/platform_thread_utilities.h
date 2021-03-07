#ifndef AL_PLATFORM_THREAD_UTILITIES_H
#define AL_PLATFORM_THREAD_UTILITIES_H

#include <cstdint>

namespace al::engine
{
    using ThreadHandle = void*;

    bool set_thread_affinity_mask(ThreadHandle threadHandle, uint64_t mask) noexcept;
    bool set_thread_highest_priority(ThreadHandle threadHandle) noexcept;
    ThreadHandle get_current_thread_handle() noexcept;
}

#endif
