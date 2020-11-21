#ifndef AL_ENGINE_CONFIG_H
#define AL_ENGINE_CONFIG_H

#include <cstddef>
#include <chrono>

#include "utilities/constexpr_functions.h"

namespace al::engine
{
    struct EngineConfig
    {
        // Memory Manager settings
        static constexpr std::size_t                MEMORY_SIZE{ gigabytes<std::size_t>(1) };

        // File System settings
        static constexpr std::size_t                MAX_FILE_HANDLES{ 4096 };

        // Job System settings
        static constexpr std::size_t                MAX_JOBS{ 1024 };
        static constexpr std::chrono::milliseconds  JOB_THREAD_SLEEP_TIME{ 5 };
    };
}

#endif
