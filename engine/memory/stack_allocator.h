#ifndef AL_STACK_ALLOCATOR_H
#define AL_STACK_ALLOCATOR_H

#include <cstddef>

#include "allocator_base.h"

namespace al::engine
{
    class StackAllocator : public AllocatorBase
    {
    public:
        StackAllocator() noexcept;
        ~StackAllocator() noexcept;

        virtual [[nodiscard]] std::byte* allocate(std::size_t memorySizeBytes) noexcept;

        void initialize(std::byte* memory, std::size_t memorySizeBytes) noexcept;
        void free_to_pointer(std::byte* ptr) noexcept;

    private:
        std::byte* memory;
        std::byte* memoryLimit;
        std::byte* top;
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
        if (!memory) return nullptr;
        if ((memoryLimit - top) < memorySizeBytes) return nullptr;

        std::byte* result = top;
        top += memorySizeBytes;
        return result;
    }

    void StackAllocator::free_to_pointer(std::byte* ptr) noexcept
    {
        if (ptr >= memory && ptr < memoryLimit) return;
        top = ptr;
    }
}

#endif
