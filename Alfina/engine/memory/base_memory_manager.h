#ifndef __ALFINA_BASE_MEMORY_MANAGER_H__
#define __ALFINA_BASE_MEMORY_MANAGER_H__

#include <functional>
#include <cstdlib>

#include "engine/engine_utilities/error_info.h"
#include "stack_allocator.h"

namespace al::engine
{
	class MemoryManager
	{
	public:
		MemoryManager() = default;
		~MemoryManager();

		ErrorInfo		init(size_t stackSizeBytes);
		StackAllocator* get_stack();

	private:
		StackAllocator stack;
	};

	MemoryManager::~MemoryManager()
	{
		auto dealloc = [](uint8_t* ptr) -> void { std::free(ptr); };
		stack.terminate(std::ref(dealloc));
	}

	ErrorInfo MemoryManager::init(size_t stackSizeBytes)
	{
		auto alloc = [](size_t sizeBytes) -> uint8_t* { return static_cast<uint8_t*>(std::malloc(sizeBytes)); };
		ErrorInfo error = stack.init(stackSizeBytes, std::ref(alloc));
		return error;
	}

	StackAllocator* MemoryManager::get_stack()
	{
		return &stack;
	}
}

#endif
