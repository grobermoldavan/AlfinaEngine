
#include "platform_threads_win32.h"

namespace al
{
    PlatformThreadId platform_get_current_thread_id()
    {
        return ::GetThreadId(::GetCurrentThread());
    }
}
