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
        static void initialize() noexcept;
        static void terminate() noexcept;

        MemoryManager() noexcept;
        ~MemoryManager() noexcept;

        static StackAllocator* get_stack() noexcept;
        static PoolAllocator* get_pool() noexcept;

    private:
        constexpr static std::size_t MEMORY_SIZE = gigabytes<std::size_t>(1);

        static MemoryManager instance;

        StackAllocator stack;
        PoolAllocator pool;
        bool isInitialized;
        std::byte* memory;
    };

    MemoryManager MemoryManager::instance;

    MemoryManager::MemoryManager() noexcept
        : isInitialized{ false }
        , memory{ nullptr }
    { }

    MemoryManager::~MemoryManager() noexcept
    { }

    StackAllocator* MemoryManager::get_stack() noexcept
    {
        return &instance.stack;
    }

    PoolAllocator* MemoryManager::get_pool() noexcept
    {
        return &instance.pool;
    }

    void MemoryManager::initialize() noexcept
    {
        if (instance.isInitialized) return;

        instance.memory = static_cast<std::byte*>(std::malloc(MEMORY_SIZE));
        instance.stack.initialize(instance.memory, MEMORY_SIZE);
        instance.pool.initialize({
            BucketDescrition{ 8, 1024 },
            BucketDescrition{ 16, 512 },
            BucketDescrition{ 32, 256 },
            BucketDescrition{ 64, 128 }
        });
    }

    void MemoryManager::terminate() noexcept
    {
        if (!instance.isInitialized) return;

        std::free(instance.memory);
    }

    [[nodiscard]] std::byte* stack_alloc(std::size_t memorySizeBytes)
    {
        return MemoryManager::get_stack()->allocate(memorySizeBytes);
    }
}

#endif
