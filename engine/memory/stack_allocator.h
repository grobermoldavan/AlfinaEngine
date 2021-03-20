#ifndef AL_STACK_ALLOCATOR_H
#define AL_STACK_ALLOCATOR_H

#include <cstddef>
#include <atomic>

#include "allocator_base.h"

// @NOTE : This allocator is thread-safe

namespace al::engine
{
    struct StackAllocator
    {
        void* memory;
        void* memoryLimit;
        std::atomic<void*> top;
    };

    void                construct   (StackAllocator* stack, void* memory, std::size_t memorySizeBytes);
    void                destruct    (StackAllocator* stack);
    [[nodiscard]] void* allocate    (StackAllocator* stack, std::size_t memorySizeBytes);
    void                deallocate  (StackAllocator* stack, void* ptr, std::size_t memorySizeBytes);

    AllocatorBindings get_allocator_bindings(StackAllocator* stack);
}

#endif
