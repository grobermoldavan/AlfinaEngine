#ifndef AL_MEMORY_BUFFER_H
#define AL_MEMORY_BUFFER_H

#include "engine/types.h"
#include "enums.h"

namespace al
{
    struct MemoryBuffer
    {

    };

    struct MemoryBufferCreateInfo
    {
        uSize sizeBytes;
        MemoryBufferUsageFlags usage;
    };
}

#endif
