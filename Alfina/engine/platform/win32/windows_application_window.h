#ifndef __ALFINA_WIN32_APPLICATION_WINDOW_H__
#define __ALFINA_WIN32_APPLICATION_WINDOW_H__

#include <thread>
#include <mutex>

namespace al::engine
{
    struct Win32ApplicationWindow : public ApplicationWindow
    {
		mutable std::mutex inputReadMutex;

        std::thread windowThread;
        HWND        hwnd;
        HGLRC       hglrc;
    };
}

#endif