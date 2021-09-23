#ifndef AL_ALLOCATOR_BINDINGS_H
#define AL_ALLOCATOR_BINDINGS_H

#include "engine/types.h"

namespace al
{
    struct AllocatorBindings
    {
        void* (*allocate)(void* allocator, uSize memorySizeBytes, uSize alignmentBytes);
        void (*deallocate)(void* allocator, void* ptr, uSize memorySizeBytes);
        void* allocator;
    };
}

#endif
