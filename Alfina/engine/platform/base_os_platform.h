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

	struct ApplicationWindowInput
	{
		enum GeneralInputFlags
		{
			CLOSE_BUTTON_PRESSED
		};

		Flags32 generalInput;
	};

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