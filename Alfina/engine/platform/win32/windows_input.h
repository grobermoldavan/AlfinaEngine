#ifndef __ALFINA_WINDOWS_INPUT_H__
#define __ALFINA_WINDOWS_INPUT_H__

#include <windows.h>

namespace al::engine
{
	bool process_mouse_input	(Win32ApplicationWindow* window, UINT message, WPARAM wParam, LPARAM lParam);
	bool process_keyboard_input	(Win32ApplicationWindow* window, UINT message, WPARAM wParam, LPARAM lParam);
}

#if defined(AL_UNITY_BUILD)
#	include "windows_input.cpp"
#else

#endif

#endif