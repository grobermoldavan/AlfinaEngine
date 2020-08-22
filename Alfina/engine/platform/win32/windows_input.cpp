#if defined(AL_UNITY_BUILD)

#else
#	include "windows_input.h"
#endif

#include "Windowsx.h"

#include "windows_application_window.h"

namespace al::engine
{
	bool process_mouse_input(Win32ApplicationWindow* window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		#define LOGMSG(msg, ...) AL_LOG_SHORT(Logger::Type::MESSAGE, #msg, __VA_ARGS__)

		switch (message)
		{
			case WM_LBUTTONDOWN:
			{
				window->input.mouse.buttons.set_flag(ApplicationWindowInput::MouseInputFlags::LMB_PRESSED);
				LOGMSG(WM_LBUTTONDOWN)
				break;
			}
			case WM_LBUTTONUP:
			{
				window->input.mouse.buttons.clear_flag(ApplicationWindowInput::MouseInputFlags::LMB_PRESSED);
				LOGMSG(WM_LBUTTONUP)
				break;
			}
			case WM_MBUTTONDOWN:
			{
				window->input.mouse.buttons.set_flag(ApplicationWindowInput::MouseInputFlags::MMB_PRESSED);
				LOGMSG(WM_MBUTTONDOWN)
				break;
			}
			case WM_MBUTTONUP:
			{
				window->input.mouse.buttons.clear_flag(ApplicationWindowInput::MouseInputFlags::MMB_PRESSED);
				LOGMSG(WM_MBUTTONUP)
				break;
			}
			case WM_RBUTTONDOWN:
			{
				window->input.mouse.buttons.set_flag(ApplicationWindowInput::MouseInputFlags::RMB_PRESSED);
				LOGMSG(WM_RBUTTONDOWN)
				break;
			}
			case WM_RBUTTONUP:
			{
				window->input.mouse.buttons.clear_flag(ApplicationWindowInput::MouseInputFlags::RMB_PRESSED);
				LOGMSG(WM_RBUTTONUP)
				break;
			}
			case WM_XBUTTONDOWN:
			{
				LOGMSG(WM_XBUTTONDOWN)
				break;
			}
			case WM_XBUTTONUP:
			{
				LOGMSG(WM_XBUTTONUP)
				break;
			}
			case WM_MOUSEMOVE:
			{
				break;
			}
			case WM_MOUSEWHEEL:
			{
				LOGMSG(WM_MOUSEWHEEL, " ", GET_WHEEL_DELTA_WPARAM(wParam))
				window->input.mouse.wheel += GET_WHEEL_DELTA_WPARAM(wParam);
				break;
			}
			default:
			{
				return false;
			}
		}

		window->input.mouse.x = GET_X_LPARAM(lParam);
		window->input.mouse.y = GET_Y_LPARAM(lParam);

		return true;

		#undef LOGMSG
	}

	bool process_keyboard_input(Win32ApplicationWindow* window, UINT message, WPARAM wParam, LPARAM lParam)
	{
	
		return false;
	}
}
