#include "utilities/constexpr_functions.h"

#include <stdio.h>
#include <functional>
#include <cassert>

namespace al::engine
{
	Root::Root(const RootInitInfo& initInfo)
		: window	{ nullptr }
		, fileSystem{ nullptr }
		, renderer	{ nullptr }
	{
		// @TODO : log errors

		// Initialize memory
		constructionResult = memory.init(initInfo.memoryBytes);
		if (is_error(constructionResult))
		{
			return;
		}

		// @NOTE : std::ref helps avoid heap allocations while creating std::function
		auto alloc = [&](size_t bytes) -> uint8_t* { return memory.get_stack()->allocate(bytes); };
		auto refAlloc = std::ref(alloc);

		// Create application window
		constructionResult = create_application_window(&window, initInfo.windowProperties, refAlloc);
		if (is_error(constructionResult))
		{
			return;
		}
		
		// Create file system
		constructionResult = create_file_system(&fileSystem, refAlloc);
		if (is_error(constructionResult))
		{
			return;
		}

		{	// test file read
			uint8_t* top = memory.get_stack()->get_current_top();
			FileHandle fh;
			fileSystem->read_file("Assets\\test.txt", &fh, refAlloc);
			if (fh.get_data())
				printf("Data is : %s\n", fh.get_data());
			else
				printf("Unable to read file.");
			memory.get_stack()->free_to_pointer(top);
		}

		// Create renderer
		constructionResult = create_renderer(&renderer, window, refAlloc);
		if (is_error(constructionResult))
		{
			return;
		}

		// Create sound system
		constructionResult = create_sound_system(&sound, window, fileSystem, refAlloc);
		if (is_error(constructionResult))
		{
			return;
		}
		sound->init(initInfo.soundParameters);
	}

	Root::~Root()
	{	
		// @TODO : log errors

		ErrorInfo error;

		auto dealloc = [&](uint8_t* ptr) -> void { return memory.get_stack()->free_to_pointer(ptr); };
		auto refDealloc = std::ref(dealloc);

		// Destroy renderer
		if (renderer)
		{
			error = destroy_renderer(renderer, refDealloc);
		}

		// Destroy file system
		if (fileSystem)
		{
			error = destroy_file_system(fileSystem, refDealloc);
		}

		// Destroy application window
		if (window)
		{
			error = destroy_application_window(window, refDealloc);
		}

		// Destroy sound system
		if (sound)
		{
			error = destroy_sound_system(sound, refDealloc);
		}
	}

	ErrorInfo Root::get_construction_result()
	{
		return constructionResult;
	}

	MemoryManager* Root::get_memory_manager()
	{
		return &memory;
	}

	ApplicationWindow* Root::get_window()
	{
		return window;
	}

	FileSystem* Root::get_file_system()
	{
		return fileSystem;
	}

	Renderer* Root::get_renderer()
	{
		return renderer;
	}

	SoundSystem* Root::get_sound_system()
	{
		return sound;
	}

	void Root::get_input(ApplicationWindowInput* inputBuffer)
	{
		window->get_input(inputBuffer);
	}
}
