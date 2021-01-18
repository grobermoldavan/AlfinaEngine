
#include "stack_allocator.h"

#include "memory_common.h"

namespace al::engine
{
    StackAllocator::StackAllocator() noexcept
        : memory{ nullptr }
        , memoryLimit{ nullptr }
        , top{ nullptr }
    { }

    StackAllocator::~StackAllocator() noexcept
    { }

    void StackAllocator::initialize(std::byte* memory, std::size_t memorySizeBytes) noexcept
    {
        this->memory = memory;
        memoryLimit = memory + memorySizeBytes;
        top = memory;
    }

    std::byte* StackAllocator::allocate(std::size_t memorySizeBytes) noexcept
    {
        // @NOTE :  Can't use al_assert here because it writes to the logger, which could not be initialized at this point in time
        // @TODO :  Add another assert macro, which does not write to the logger
        // al_assert(memory);
        std::byte* result = nullptr;
        while (true)
        {
            std::byte* currentTop = top.load(std::memory_order_relaxed);
            std::byte* currentTopAligned = align_pointer(currentTop);
            if ((memoryLimit - currentTopAligned) < memorySizeBytes)
            {
                break;
            }
            std::byte* newTop = currentTopAligned + memorySizeBytes;
            const bool casResult = top.compare_exchange_strong(currentTop, newTop);
            if (casResult)
            {
                result = currentTopAligned;
                break;
            }
        }
        return result;
    }

    void StackAllocator::deallocate(std::byte*, std::size_t) noexcept
    {
        // Can't deallocate with stack allocator
    }

    void StackAllocator::free_to_pointer(std::byte* ptr) noexcept
    {
        // @NOTE :  Can't use al_assert here because it writes to the logger, which could not be initialized at this point in time
        // @TODO :  Add another assert macro, which does not write to the logger
        // al_assert(memory);
        if (ptr >= memory && ptr < memoryLimit) return;
        top = ptr;
    }
}
