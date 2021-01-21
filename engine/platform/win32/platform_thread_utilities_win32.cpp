
#include "engine/platform/win32/win32_backend.h"
#include "engine/platform/platform_thread_utilities.h"

namespace al::engine
{
    bool set_thread_affinity_mask(ThreadHandle threadHandle, uint64_t mask) noexcept
    {
        return ::SetThreadAffinityMask(threadHandle, static_cast<DWORD_PTR>(mask));
    }

    ThreadHandle get_current_thread_handle() noexcept
    {
        return ::GetCurrentThread();
    }
}
