#ifndef AL_CONFIG_H
#define AL_CONFIG_H

#include "engine/types.h"

namespace al
{
    struct EngineConfig
    {
        static constexpr const char* ENGINE_NAME            = "Alfina Engine";
        static constexpr uSize DEFAULT_MEMORY_ALIGNMENT     = 8;
        static constexpr uSize POOL_ALLOCATOR_MAX_BUCKETS   = 8;
        static constexpr uSize STACK_ALLOCATOR_MEMORY_SIZE  = 1024 * 1024; // 1 MB
        static constexpr uSize POOL_ALLOCATOR_MEMORY_SIZE   = 1024 * 1024; // 1 MB
        static constexpr uSize PLATFORM_FILE_PATH_SIZE      = 256;
    };
}

#endif
