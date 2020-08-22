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
		
		Dispensable<uint32_t> 	resolutionWidth;
		Dispensable<uint32_t> 	resolutionHeight;
		Dispensable<uint32_t> 	width;
		Dispensable<uint32_t> 	height;
		ScreenMode				isFullScreen;
		
		Dispensable<char*> 		name;
	};

	// This struct gets changed by OS.
	// For user this is read-only.
	struct ApplicationWindowInput
	{
		enum GeneralInputFlags
		{
			CLOSE_BUTTON_PRESSED
		};

		enum MouseInputFlags
		{
			LMB_PRESSED,
			RMB_PRESSED,
			MMB_PRESSED
		};

		Flags32 generalInput;
		
		struct {
			Flags32 buttons;
			int32_t x;
			int32_t y;
			int32_t wheel;
		} mouse { };
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
		ApplicationWindowInput 	input;
		ApplicationWindowState  state;
		Renderer* 				renderer;
	};
	
	extern ErrorInfo	create_application_window	(const WindowProperties&, ApplicationWindow**);
	extern ErrorInfo	destroy_application_window	(ApplicationWindow*);

	class Renderer;

	extern ErrorInfo	create_renderer				(Renderer**, ApplicationWindow*);
	extern ErrorInfo	destroy_renderer			(Renderer*);
}

#endif