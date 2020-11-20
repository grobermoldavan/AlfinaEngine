#ifndef AL_SYSTEM_ALLOCATOR_H
#define AL_SYSTEM_ALLOCATOR_H

#include <stdlib.h>

#include "allocator_base.h"

// @NOTE :  This allocator is thread-safe

namespace al::engine
{
    class SystemAllocator : public AllocatorBase
    {
    public:
        virtual [[nodiscard]] std::byte* allocate(std::size_t memorySizeBytes) noexcept override;
        virtual void deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept override;
    };

    [[nodiscard]] std::byte* SystemAllocator::allocate(std::size_t memorySizeBytes) noexcept
    {
        std::byte* result = static_cast<std::byte*>(malloc(memorySizeBytes));
        return result;
    }

    void SystemAllocator::deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept
    {
        free(ptr);
    }
}

#endif
