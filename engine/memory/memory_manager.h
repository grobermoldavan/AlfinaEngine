#ifndef AL_MEMORY_MANAGER_H
#define AL_MEMORY_MANAGER_H

#include <cstddef>
#include <cstdlib>

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

        void initialize() noexcept;
        void terminate() noexcept;

        StackAllocator* get_stack() noexcept;
        PoolAllocator* get_pool() noexcept;

    private:
        constexpr static std::size_t MEMORY_SIZE = gigabytes<std::size_t>(1);

        StackAllocator stack;
        PoolAllocator pool;
        bool isInitialized;
        std::byte* memory;
    };

    MemoryManager::MemoryManager() noexcept
        : isInitialized{ false }
        , memory{ nullptr }
    { }

    MemoryManager::~MemoryManager() noexcept
    { }

    void MemoryManager::initialize() noexcept
    {
        memory = static_cast<std::byte*>(std::malloc(MEMORY_SIZE));
        stack.initialize(memory, MEMORY_SIZE);
        pool.initialize({
            BucketDescrition{ 8, 1024 },
            BucketDescrition{ 16, 512 },
            BucketDescrition{ 32, 256 },
            BucketDescrition{ 64, 128 }
        }, &stack);
    }

    void MemoryManager::terminate() noexcept
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
