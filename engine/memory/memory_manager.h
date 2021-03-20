#ifndef AL_MEMORY_MANAGER_H
#define AL_MEMORY_MANAGER_H

#include <cstddef>
#include <cstdlib>

#include "stack_allocator.h"
#include "pool_allocator.h"

namespace al::engine
{
    extern struct MemoryManager* gMemoryManager;

    struct MemoryManager
    {
        StackAllocator  stack;
        PoolAllocator   pool;
        void*           memory;
    };

    void construct(MemoryManager* manager);
    void destruct(MemoryManager* manager);
}

#endif
