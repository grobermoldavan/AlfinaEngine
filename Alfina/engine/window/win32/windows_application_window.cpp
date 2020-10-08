
#include "windows_application_window.h"
#include "windows_input.h"

namespace al::engine
{
	ErrorInfo create_application_window(ApplicationWindow** window, const WindowProperties& properties, std::function<uint8_t* (size_t sizeBytes)> allocate)
	{
		if (!window)
		{
			return { ErrorInfo::Code::INCORRECT_INPUT_DATA, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::NULL_PTR_PROVIDED] };
		}

		// Allocate memory and construct object
		Win32ApplicationWindow* win32window = reinterpret_cast<Win32ApplicationWindow*>(allocate(sizeof(Win32ApplicationWindow)));
		if (!win32window)
		{
			return { ErrorInfo::Code::ALLOCATION_ERROR, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::UNABLE_TO_ALLOCATE_MEMORY] };
		}

		win32window = new(win32window) Win32ApplicationWindow(properties);
		*window = win32window;

		return { ErrorInfo::Code::ALL_FINE, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::ALL_FINE] };
	}

	ErrorInfo destroy_application_window(ApplicationWindow* window, std::function<void(uint8_t* ptr)> deallocate)
	{
		if (!window)
		{
			return { ErrorInfo::Code::INCORRECT_INPUT_DATA, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::NULL_PTR_PROVIDED] };
		}

		Win32ApplicationWindow* win32window = static_cast<Win32ApplicationWindow*>(window);

		// Destruct and deallocate window
		win32window->~Win32ApplicationWindow();
		deallocate(reinterpret_cast<uint8_t*>(win32window));

		return { ErrorInfo::Code::ALL_FINE, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::ALL_FINE] };
	}

	Win32ApplicationWindow::Win32ApplicationWindow(const WindowProperties& properties)
		: ApplicationWindow{ properties }
	{ 
		// Create window input read mutex
		inputReadMutex = ::CreateMutex(	NULL,	// default security attributes
										FALSE,	// initially not owned
										NULL);	// unnamed mutex
		
		// Start window thread and wait for initialization
		threadArg =
		{
			this,
			::CreateEvent(NULL, TRUE, FALSE, TEXT("InitEvent"))
		};
		windowThread = ::CreateThread(NULL, 0, window_update, &threadArg, 0, NULL);
		::WaitForSingleObject(threadArg.initEvent, INFINITE);
		::CloseHandle(threadArg.initEvent);
	}

	Win32ApplicationWindow::~Win32ApplicationWindow()
	{
		// Set flag to notify window thread that window is closing
		state.flags.set_flag(Win32WindowState::StateFlags::WINDOW_CLOSED);

		// Wait for thread
		::WaitForSingleObject(windowThread, INFINITE);
		::CloseHandle(windowThread);

		// Close handle to a mutex
		::CloseHandle(inputReadMutex);
	}

	void Win32ApplicationWindow::get_input(ApplicationWindowInput* inputBuffer)
	{
		// Wait for mutex
		::WaitForSingleObject(inputReadMutex, INFINITE);
		// Copy data
		std::memcpy(inputBuffer, &input, sizeof(ApplicationWindowInput));
		// Release mutex
		::ReleaseMutex(inputReadMutex);
	}

	LRESULT CALLBACK window_proc(	HWND	hwnd,
									UINT	message,
									WPARAM	wParam,
									LPARAM	lParam)
	{
		// Get pointer to a window
		Win32ApplicationWindow* win32window = (Win32ApplicationWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

		// Try process mouse input
		if (process_mouse_input(win32window, message, wParam, lParam))
		{
			return 0;
		}

		// Try process keyboard input
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
			win32window->input.generalInput.set_flag(ApplicationWindowInput::GeneralInputFlags::CLOSE_BUTTON_PRESSED);
			return 0;
		}
		
		return ::DefWindowProc(hwnd, message, wParam, lParam);
	}

	DWORD window_update(LPVOID voidArg)
	{
		// @TODO : add error handling

		WindowThreadArg* windowArgPtr = static_cast<WindowThreadArg*>(voidArg);
		Win32ApplicationWindow* win32window = windowArgPtr->window;

		HMODULE moduleHandle = ::GetModuleHandle(NULL);

		const char* WIN_NAME = win32window->properties.name;

		WNDCLASS wc = {};

		wc.lpfnWndProc		= window_proc;
		wc.hInstance		= moduleHandle;
		wc.lpszClassName	= WIN_NAME;
		wc.hCursor			= ::LoadCursor(NULL, IDC_ARROW);

		const bool isClassRegistered = ::RegisterClass(&wc);

		constexpr DWORD windowedStyle = 
			WS_OVERLAPPED	|			// Window has title and borders
			WS_SYSMENU		|			// Add sys menu (close button)
			WS_MINIMIZEBOX	;			// Add minimize button

		constexpr DWORD fullscreenStyle = WS_POPUP;

		HWND hwnd = ::CreateWindowEx(
			0,                              										// Optional window styles.
			WIN_NAME,               												// Window class
			WIN_NAME,    															// Window text
			win32window->properties.isFullscreen ? fullscreenStyle : windowedStyle,	// Window styles
			CW_USEDEFAULT, 															// Position X
			CW_USEDEFAULT, 															// Position Y
			win32window->properties.width,											// Size X
			win32window->properties.height,											// Size Y
			NULL,       															// Parent window    
			NULL,       															// Menu
			moduleHandle,  															// Instance handle
			NULL        															// Additional application data
		);

		win32window->hwnd = hwnd;
		::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(win32window));

		::ShowWindow(hwnd, win32window->properties.isFullscreen ? SW_MAXIMIZE : SW_SHOW);
		::UpdateWindow(hwnd);

		::SetEvent(windowArgPtr->initEvent);
		// After this windowArgPtr->initEvent will not be valid (handle will be closed)

		MSG msg = {};
		while (msg.message != WM_QUIT)
		{
			if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}

			if (win32window->state.flags.get_flag(Win32WindowState::StateFlags::WINDOW_CLOSED))
			{
				::PostQuitMessage(0);
			}
		}

		return 0;
	}
}