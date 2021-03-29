#ifndef AL_PLATFORM_WINDOW_H
#define AL_PLATFORM_WINDOW_H

#include "engine/types.h"
#include "engine/utilities/utilities.h"

namespace al
{
    struct PlatformWindow;

    struct PlatformWindowInitData
    {
        const char* name;
        bool isFullscreen;
        bool isResizable;
        u32 width;
        u32 height;
    };

    void    platform_window_construct               (PlatformWindow* window, const PlatformWindowInitData& initData);
    void    platform_window_destruct                (PlatformWindow* window);
    void    platform_window_process                 (PlatformWindow* window);
    bool    platform_window_is_close_button_pressed (PlatformWindow* window);
    u32     platform_window_get_current_width       (PlatformWindow* window);
    u32     platform_window_get_current_height      (PlatformWindow* window);
    void    platform_window_set_resize_callback     (PlatformWindow* window, const Function<void()>& callback);
}

#endif
