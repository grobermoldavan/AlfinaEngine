#ifndef __ALFINA_ROOT_H__
#define __ALFINA_ROOT_H__

#define AL_WIN_32

#include "engine/engine_utilities/error_info.h"

#include "engine/memory/base_memory_manager.h"

#include "engine/window/base_application_window.h"
#ifdef AL_WIN_32
#	include "engine/window/win32/windows_application_window.h"
#else
#	error Unsupported plaform
#endif

#include "engine/file_system/base_file_system.h"
#ifdef AL_WIN_32
#	include "engine/file_system/win32/windows_file_system.h"
#else
#	error Unsupported plaform
#endif

#include "engine/renderer/base_renderer.h"
#ifdef AL_WIN_32
#	include "engine/renderer/win32/windows_opengl_renderer.h"
#else
#	error Unsupported plaform
#endif

#include "engine/sound_system/base_sound_system.h"
#ifdef AL_WIN_32
#	include "engine/sound_system/win32/windows_sound_system.h"
#else
#	error Unsupported plaform
#endif

namespace al::engine
{
	struct RootInitInfo
	{
		// Properties of the window
		WindowProperties windowProperties;
		// Properties of the sound
		SoundParameters soundParameters;
		// Amound of memory to be allocated for the root
		size_t memoryBytes;
	};

	class Root
	{
	public:
		Root(const RootInitInfo& initInfo);
		~Root();

		ErrorInfo				get_construction_result();
		MemoryManager*			get_memory_manager();
		ApplicationWindow*		get_window();
		FileSystem*				get_file_system();
		Renderer*				get_renderer();
		SoundSystem*			get_sound_system();
		void					get_input(ApplicationWindowInput* inputBuffer);

	private:
		MemoryManager		memory;
		ApplicationWindow*	window;
		FileSystem*			fileSystem;
		Renderer*			renderer;
		SoundSystem*		sound;

		ErrorInfo constructionResult;
	};
}

#include "root.cpp"

#endif
