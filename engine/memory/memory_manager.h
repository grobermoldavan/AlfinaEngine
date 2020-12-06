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

        static MemoryManager* get()
        {
            static MemoryManager instance;
            return &instance;
        }

    private:
        StackAllocator stack;
        PoolAllocator pool;
        std::byte* memory;
    };

    MemoryManager::MemoryManager() noexcept
        : memory{ static_cast<std::byte*>(std::malloc(EngineConfig::MEMORY_SIZE)) }
    {
        stack.initialize(memory, EngineConfig::MEMORY_SIZE);
        // @TODO : change this hardcoded pool initialization to
        //         dynamic, which counts each bucket memory size
        //         based on some user settings
        pool.initialize({
            bucket_desc(64                          , megabytes<std::size_t>(128)),
            bucket_desc(128                         , megabytes<std::size_t>(128)),
            bucket_desc(256                         , megabytes<std::size_t>(128)),
            bucket_desc(kilobytes<std::size_t>(1)   , megabytes<std::size_t>(128)),
            bucket_desc(kilobytes<std::size_t>(4)   , megabytes<std::size_t>(256)),
        }, &stack);
    }

    MemoryManager::~MemoryManager() noexcept
    {
        std::free(memory);
    }

    StackAllocator* MemoryManager::get_stack() noexcept
    {
        return &stack;
    }

    PoolAllocator* MemoryManager::get_pool() noexcept
    {
        return &pool;
    }
}

#endif
