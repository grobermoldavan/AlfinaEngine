#ifndef __ALFINA_WIN32_APPLICATION_WINDOW_H__
#define __ALFINA_WIN32_APPLICATION_WINDOW_H__

#include <thread>

namespace al::engine
{
    struct Win32ApplicationWindow : public ApplicationWindow
    {
        std::thread windowThread;
        HWND        hwnd;
        HGLRC       hglrc;
    };
}

#endif