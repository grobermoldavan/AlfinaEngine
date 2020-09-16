#ifndef __ALFINA_WINDOWS_PLATFORM_H__
#define __ALFINA_WINDOWS_PLATFORM_H__

#include <windows.h>

#include <future>

#include "../base_os_platform.h"

#include "windows_application_window.h"

namespace al::engine
{
	LRESULT CALLBACK	window_proc					(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void				window_update				(Win32ApplicationWindow* win32window, std::promise<void> creation_promise);
	ErrorInfo			create_application_window	(const WindowProperties& properties, ApplicationWindow** window);
	ErrorInfo			destroy_application_window	(ApplicationWindow* window);
	ErrorInfo			get_window_inputs			(const ApplicationWindow*, ApplicationWindowInput*);
}

#if defined(AL_UNITY_BUILD)
#	include "windows_platform.cpp"
#else

#endif

#endif
