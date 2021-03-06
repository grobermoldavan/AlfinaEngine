#ifndef AL_MEMORY_MANAGER_H
#define AL_MEMORY_MANAGER_H

#include <cstddef>
#include <cstdlib>

#include "stack_allocator.h"
#include "pool_allocator.h"

namespace al::engine
{
    class MemoryManager
    {
    public:
        static void             construct   () noexcept;
        static void             destruct    () noexcept;
        static StackAllocator*  get_stack   () noexcept;
        static PoolAllocator*   get_pool    () noexcept;

        static void log_memory_init_info() noexcept;

    private:
        static MemoryManager instance;

        StackAllocator stack;
        PoolAllocator pool;
        std::byte* memory;

        MemoryManager() noexcept;
        ~MemoryManager() noexcept;
    };
}

#endif
