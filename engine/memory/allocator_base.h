#ifndef AL_ALLOCATOR_BASE_H
#define AL_ALLOCATOR_BASE_H

#include <cstddef> // for std::size_t

namespace al::engine
{
    struct AllocatorBindings
    {
        [[nodiscard]] void* (*allocate)(void* allocator, std::size_t memorySizeBytes);
        void (*deallocate)(void* allocator, void* ptr, std::size_t memorySizeBytes);
        void* allocator;
    };
}

#endif
