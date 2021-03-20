
#include "stack_allocator.h"

#include "memory_common.h"

namespace al::engine
{
    void construct(StackAllocator* stack, void* memory, std::size_t memorySizeBytes)
    {
        stack->memory = memory;
        stack->memoryLimit = static_cast<uint8_t*>(memory) + memorySizeBytes;
        stack->top = memory;
    }

    void destruct(StackAllocator* stack)
    {
        // nothing
    }

    [[nodiscard]] void* allocate(StackAllocator* stack, std::size_t memorySizeBytes)
    {
        // @NOTE :  Can't use al_assert here because it writes to the logger, which could not be initialized at this point in time
        // @TODO :  Add another assert macro, which does not write to the logger
        // al_assert(memory);
        void* result = nullptr;
        while (true)
        {
            void* currentTop = std::atomic_load_explicit(&stack->top, std::memory_order_relaxed);
            uint8_t* currentTopAligned = align_pointer(static_cast<uint8_t*>(currentTop));
            if ((static_cast<uint8_t*>(stack->memoryLimit) - currentTopAligned) < memorySizeBytes)
            {
                break;
            }
            uint8_t* newTop = currentTopAligned + memorySizeBytes;
            const bool casResult = std::atomic_compare_exchange_strong(&stack->top, &currentTop, newTop);
            if (casResult)
            {
                result = currentTopAligned;
                break;
            }
        }
        return result;
    }

    void deallocate(StackAllocator* stack, void* ptr, std::size_t memorySizeBytes)
    {
        // Can't deallocate with stack allocator
    }

    AllocatorBindings get_allocator_bindings(StackAllocator* stack)
    {
        return
        {
            .allocate = [](void* allocator, std::size_t size){ return allocate(static_cast<StackAllocator*>(allocator), size); },
            .deallocate = [](void* allocator, void* ptr, std::size_t size){ deallocate(static_cast<StackAllocator*>(allocator), ptr, size); },
            .allocator = stack
        };
    }
}
