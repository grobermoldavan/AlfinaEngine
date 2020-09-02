#ifndef __ALFINA_WIN32_APPLICATION_WINDOW_H__
#define __ALFINA_WIN32_APPLICATION_WINDOW_H__

#include <thread>
#include <mutex>

namespace al::engine
{
	struct Win32WindowState
	{
		enum StateFlags
		{
			WINDOW_CLOSED
		};

		Flags32 flags;
	};

    struct Win32ApplicationWindow : public ApplicationWindow
    {
		mutable std::mutex inputReadMutex;

		Win32WindowState	state;
		std::thread			windowThread;
		HWND				hwnd;
		HGLRC				hglrc;
    };
}

#endif