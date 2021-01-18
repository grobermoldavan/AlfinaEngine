#ifndef AL_MEMORY_MANAGER_H
#define AL_MEMORY_MANAGER_H

#include <cstddef>
#include <cstdlib>

#include "engine/config/engine_config.h"

#include "stack_allocator.h"
#include "pool_allocator.h"

#include "utilities/constexpr_functions.h"

namespace al::engine
{
    class MemoryManager
    {
    public:
        MemoryManager() noexcept;
        ~MemoryManager() noexcept;

        StackAllocator* get_stack() noexcept;
        PoolAllocator* get_pool() noexcept;

        static MemoryManager* get() noexcept;

    private:
        StackAllocator stack;
        PoolAllocator pool;
        std::byte* memory;
    };
}

#endif
