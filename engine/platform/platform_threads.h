#ifndef AL_PLATFORM_THREADS_H
#define AL_PLATFORM_THREADS_H

#include "engine/types.h"

namespace al
{
    using PlatformThreadId = u64;

    PlatformThreadId platform_get_current_thread_id();
}

#endif
