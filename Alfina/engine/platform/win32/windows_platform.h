#ifndef __ALFINA_WINDOWS_PLATFORM_H__
#define __ALFINA_WINDOWS_PLATFORM_H__

#include <windows.h>


#include <map>
#include <future>

#include "engine/platform/base_os_platform.h"
#include "engine/platform/win32/windows_application_window.h"

namespace al::engine
{
	std::map<HWND, Win32ApplicationWindow*> hWndToWindowMap;

	LRESULT CALLBACK	window_proc					(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void				window_update				(Win32ApplicationWindow* win32window, std::promise<void> creation_promise);
	ErrorInfo			create_application_window	(const WindowProperties& properties, ApplicationWindow** window);
	ErrorInfo			destroy_application_window	(ApplicationWindow* window);
}

#if defined(AL_UNITY_BUILD)
#	include "windows_platform.cpp"
#else

#endif

#endif