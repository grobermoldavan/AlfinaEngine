#ifndef AL_PLATFORM_WINDOW_WIN32_H
#define AL_PLATFORM_WINDOW_WIN32_H

#include "platform_win32_backend.h"
#include "platform_input_win32.h"
#include "../platform_window.h"

namespace al
{
    struct PlatformWindow
    {
        PlatformInput input;
        Function<void()> resizeCallback;
        HWND handle;
        u32 width;
        u32 height;
        bool isCloseButtonPressed;
    };
}

#endif
