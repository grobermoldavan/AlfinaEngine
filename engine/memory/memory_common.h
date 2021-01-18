#ifndef AL_MEMORY_COMMON_H
#define AL_MEMORY_COMMON_H

#include "engine/config/engine_config.h"

#define al_align alignas(EngineConfig::DEFAULT_MEMORY_ALIGNMENT)

namespace al::engine
{
    template<typename T> T* align_pointer(T* ptr) noexcept;
}

#endif
