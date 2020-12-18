#ifndef AL_PLATFORM_THREAD_EVENT_H
#define AL_PLATFORM_THREAD_EVENT_H

#ifdef AL_PLATFORM_WIN32
#   include "win32/platform_thread_event_win32.h"
#else
#   error Unsupported platform
#endif

#endif
