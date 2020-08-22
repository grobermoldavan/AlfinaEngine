#if defined(AL_UNITY_BUILD)

#else
#	include "windows_platform.h"
#endif

#include <map>
#include <thread>

#include "windows_utilities.h"
#include "windows_input.h"

#include "engine/engine_utilities/asserts.h"

namespace al::engine
{
	LRESULT CALLBACK window_proc(	HWND	hWnd,
									UINT	message,
									WPARAM	wParam,
									LPARAM	lParam)
	{
		Win32ApplicationWindow* win32window = (Win32ApplicationWindow*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
		
		if (process_mouse_input(win32window, message, wParam, lParam))
		{
			return 0;
		}
		
		if (process_keyboard_input(win32window, message, wParam, lParam))
		{
			return 0;
		}

		switch (message)
		{
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
		case WM_CLOSE:
			AL_LOG_SHORT(Logger::Type::MESSAGE, "WM_CLOSE")
			win32window->input.generalInput.set_flag(ApplicationWindowInput::GeneralInputFlags::CLOSE_BUTTON_PRESSED);
			return 0;
		}

		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}

	void window_update(Win32ApplicationWindow* win32window, std::promise<void> creation_promise)
	{
		HMODULE moduleHandle = ::GetModuleHandle(NULL);
		AL_ASSERT_MSG_NO_DISCARD(moduleHandle, "Win32 :: Unable to get module handle")

		const char *WIN_NAME = win32window->properties.name.is_specified() ? win32window->properties.name : "Alfina Engine";

		WNDCLASS wc = {};

		wc.lpfnWndProc		= window_proc;
		wc.hInstance		= moduleHandle;
		wc.lpszClassName	= WIN_NAME;
		wc.hCursor			= ::LoadCursor(NULL, IDC_ARROW);

		const bool isClassRegistered = ::RegisterClass(&wc);
		AL_ASSERT_MSG_NO_DISCARD(isClassRegistered, "Win32 :: Unable to register window class")

		const bool isFullscreen = win32window->properties.screenMode == WindowProperties::ScreenMode::FULL_SCREEN;

		constexpr DWORD windowedStyle = 
			WS_OVERLAPPED	|			// Window has title and borders
			WS_SYSMENU		|			// Add sys menu (close button)
			WS_MINIMIZEBOX	;			// Add minimize button

		constexpr DWORD fullscreenStyle = WS_POPUP;

		HWND hwnd = ::CreateWindowEx(
			0,                              																// Optional window styles.
			WIN_NAME,               																		// Window class
			WIN_NAME,    																					// Window text
			isFullscreen ? fullscreenStyle : windowedStyle,													// Window styles
			CW_USEDEFAULT, 																					// Position X
			CW_USEDEFAULT, 																					// Position Y
			win32window->properties.width.is_specified() ? win32window->properties.width : CW_USEDEFAULT,   // Size X
			win32window->properties.height.is_specified() ? win32window->properties.height : CW_USEDEFAULT, // Size Y
			NULL,       																					// Parent window    
			NULL,       																					// Menu
			moduleHandle,  																					// Instance handle
			NULL        																					// Additional application data
		);

		AL_ASSERT_MSG_NO_DISCARD(hwnd, "Win32 :: Unable to create window via CreateWindowEx")

		win32window->hwnd = hwnd;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)win32window);

		::ShowWindow(hwnd, isFullscreen ? SW_MAXIMIZE : SW_SHOW);
		::UpdateWindow(hwnd);

		creation_promise.set_value();

		MSG msg = {};
		while (msg.message != WM_QUIT)
		{
			if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}

			if (win32window->state.flags.get_flag(ApplicationWindowState::StateFlags::WINDOW_CLOSED))
			{
				::PostQuitMessage(0);
			}
		}
	}

	ErrorInfo create_application_window(const WindowProperties& properties, ApplicationWindow** window)
	{
		Win32ApplicationWindow* win32window = new Win32ApplicationWindow();

		std::promise<void> creation_promise;
		std::future<void> creation_future = creation_promise.get_future();

		win32window->properties = properties;
		win32window->windowThread = std::thread{ window_update, win32window, std::move(creation_promise) };
		creation_future.get();

		create_renderer(&win32window->renderer, win32window);

		*window = static_cast<ApplicationWindow*>(win32window);

		return{ al::engine::ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo destroy_application_window(ApplicationWindow* window)
	{
		if (!window)
		{
			AL_LOG_SHORT(Logger::Type::ERROR_MSG, "No window provided")
			return{ al::engine::ErrorInfo::Code::INCORRECT_INPUT_DATA };
		}

		Win32ApplicationWindow* win32window = static_cast<Win32ApplicationWindow*>(window);

		AL_LOG_SHORT(Logger::Type::MESSAGE, "Joining window thread")

		win32window->state.flags.set_flag(ApplicationWindowState::StateFlags::WINDOW_CLOSED);
		win32window->windowThread.join();

		AL_LOG_SHORT(Logger::Type::MESSAGE, "Window thread joined")

		destroy_renderer(win32window->renderer);

		delete win32window;

		return{ al::engine::ErrorInfo::Code::ALL_FINE };
	}
}