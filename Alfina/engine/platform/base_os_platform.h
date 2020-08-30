#ifndef __ALFINA_BASE_OS_PLATFORM_H__
#define __ALFINA_BASE_OS_PLATFORM_H__

#include <cstdint>

#include "engine/engine_utilities/error_info.h"

#include "utilities/dispensable.h"
#include "utilities/flags.h"

namespace al::engine
{
	struct WindowProperties
	{
		enum ScreenMode : uint8_t
		{
			FULL_SCREEN,
			WINDOWED
		};

		Dispensable<uint32_t> 	width;
		Dispensable<uint32_t> 	height;
		ScreenMode				screenMode;
		
		Dispensable<char*> 		name;
	};

	// This struct gets changed by OS.
	// For user this is read-only.
	struct ApplicationWindowInput
	{
		enum GeneralInputFlags : uint32_t
		{
			CLOSE_BUTTON_PRESSED
		};

		enum MouseInputFlags : uint32_t
		{
			LMB_PRESSED,
			RMB_PRESSED,
			MMB_PRESSED
		};

		enum KeyboardInputFlags : uint32_t
		{
			NONE,
			Q, W, E, R, T, Y, U, I, O, P, A, S, D, F, G, H, J, K, L, Z, X, C, V, B, N, M,
			L_BRACKET, R_BRACKET, SEMICOLON, APOSTROPHE, BACKSLASH, COMMA, PERIOD, SLASH,
			TILDA, NUM_1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7, NUM_8, NUM_9, NUM_0, MINUS, EQUALS,
			TAB, CAPS_LOCK, ENTER, BACKSPACE, SPACE,
			L_SHIFT, R_SHIFT, SHIFT,
			L_CTRL, R_CTRL, CTRL,
			L_ALT, R_ALT, ALT,
			ESCAPE, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
			ARROW_UP, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT, __end
		};

		struct
		{
			Flags128 buttons;
		} keyboard { };

		struct {
			Flags32 buttons;
			int32_t x;
			int32_t y;
			int32_t wheel;
		} mouse { };

		Flags32 generalInput;
	};

	// @TODO : remove this from the base class
	struct ApplicationWindowState
	{
		enum StateFlags
		{
			WINDOW_CLOSED
		};

		Flags32 flags;
	};

	class Renderer;

	struct ApplicationWindow
	{
		WindowProperties		properties;
		Renderer* 				renderer;
		ApplicationWindowInput 	input;
		ApplicationWindowState  state;
	};
	
	extern ErrorInfo	create_application_window	(const WindowProperties&, ApplicationWindow**);
	extern ErrorInfo	destroy_application_window	(ApplicationWindow*);

	extern ErrorInfo	get_window_inputs			(const ApplicationWindow*, ApplicationWindowInput*);

	extern ErrorInfo	create_renderer				(Renderer**, ApplicationWindow*);
	extern ErrorInfo	destroy_renderer			(Renderer*);
}

#endif