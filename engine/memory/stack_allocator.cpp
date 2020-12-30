
#include "stack_allocator.h"

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
        al_assert(memory);

        std::byte* result = nullptr;

        while (true)
        {
            std::byte* currentTop = top;
            if ((memoryLimit - currentTop) < memorySizeBytes)
            {
                break;
            }

            std::byte* newTop = top + memorySizeBytes;
            const bool casResult = top.compare_exchange_strong(currentTop, newTop);
            if (casResult)
            {
                result = currentTop;
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
        al_assert(memory);
        if (ptr >= memory && ptr < memoryLimit) return;
        top = ptr;
    }
}
