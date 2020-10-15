#ifndef __ALFINA_BASE_MEMORY_MANAGER_H__
#define __ALFINA_BASE_MEMORY_MANAGER_H__

#include <functional>
#include <cstdlib>

#include "engine/engine_utilities/error_info.h"
#include "stack_allocator.h"
#include "allocator_interface.h"

namespace al::engine
{
    class MemoryManager
    {
    public:
                            MemoryManager           (size_t stackSizeBytes)         noexcept;
                            ~MemoryManager          ()                              noexcept;

        // This function can return any type of allocator. The return type will be automatically used by all systems in al::engine::Root.
        // The only requirement is that return type must be able to be used as a template parameter for al::engine::AllocatorInterface.
        // For example, this is a valid function declaration : MySuperAllocator * get_main_allocator();
        StackAllocator *    get_main_allocator      ()                              noexcept;
        StackAllocator *    get_stack               ()                              noexcept;
        ErrorInfo           get_construction_result ()                      const   noexcept;

    private:
        StackAllocator  stack;
        ErrorInfo       constructionResult;
    };

    MemoryManager::MemoryManager(size_t stackSizeBytes) noexcept
    {
        auto alloc = [](size_t sizeBytes) -> uint8_t * { return static_cast<uint8_t *>(std::malloc(sizeBytes)); };
        constructionResult = stack.init(stackSizeBytes, std::ref(alloc));
    }

    MemoryManager::~MemoryManager() noexcept
    {
        auto dealloc = [](uint8_t *ptr) -> void { std::free(ptr); };
        stack.terminate(std::ref(dealloc));
    }

    StackAllocator *MemoryManager::get_main_allocator() noexcept
    {
        return &stack;
    }

    StackAllocator *MemoryManager::get_stack() noexcept
    {
        return &stack;
    }

    ErrorInfo MemoryManager::get_construction_result() const noexcept
    {
        return constructionResult;
    }

} // namespace al::engine

#endif
