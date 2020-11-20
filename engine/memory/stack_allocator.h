#ifndef AL_STACK_ALLOCATOR_H
#define AL_STACK_ALLOCATOR_H

#include <cstddef>
#include <atomic>

#include "allocator_base.h"
#include "engine/asserts/asserts.h"

// @NOTE : This allocator is thread-safe

namespace al::engine
{
    class StackAllocator : public AllocatorBase
    {
    public:
        StackAllocator() noexcept;
        ~StackAllocator() noexcept;

        virtual [[nodiscard]] std::byte* allocate(std::size_t memorySizeBytes) noexcept;
        virtual void deallocate(std::byte*, std::size_t) noexcept override;

        void initialize(std::byte* memory, std::size_t memorySizeBytes) noexcept;
        void free_to_pointer(std::byte* ptr) noexcept;

    private:
        std::byte* memory;
        std::byte* memoryLimit;
        std::atomic<std::byte*> top;
    };

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

#endif
