#ifndef AL_STACK_ALLOCATOR_H
#define AL_STACK_ALLOCATOR_H

#include <cstddef>
#include <atomic>

#include "allocator_base.h"

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
}

#endif
