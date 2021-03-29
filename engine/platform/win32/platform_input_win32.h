#ifndef AL_PLATFORM_INPUT_WIN32_H
#define AL_PLATFORM_INPUT_WIN32_H

#include "engine/platform/win32/platform_win32_backend.h"
#include "engine/platform/platform_input.h"
#include "engine/types.h"

namespace al
{
    struct PlatformWindow;

    struct PlatformInput
    {
        struct
        {
            u64 buttons[2];
        } keyboard;
        struct
        {
            u32 buttons;
            s32 x;
            s32 y;
            s32 wheel;
        } mouse;
        PlatformWindow* window;
    };
}

#endif