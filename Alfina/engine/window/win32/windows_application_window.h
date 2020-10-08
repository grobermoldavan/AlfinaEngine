#ifndef __ALFINA_WIN32_APPLICATION_WINDOW_H__
#define __ALFINA_WIN32_APPLICATION_WINDOW_H__

#include <windows.h>

#include "../base_application_window.h"

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

	class Win32ApplicationWindow;

	struct WindowThreadArg
	{
		Win32ApplicationWindow* window;
		HANDLE					initEvent;
	};

	class Win32ApplicationWindow : public ApplicationWindow
    {
	public:
		Win32ApplicationWindow(const WindowProperties& properties);
		~Win32ApplicationWindow();

		virtual void get_input(ApplicationWindowInput* inputBuffer) override;

	public:
		Win32WindowState	state;

		WindowThreadArg		threadArg;
		HANDLE				windowThread;
		HANDLE 				inputReadMutex;
		HWND				hwnd;
		HGLRC				hglrc;
    };

	DWORD window_update(LPVOID voidArg);
	LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
}

#include "windows_application_window.cpp"

#endif
