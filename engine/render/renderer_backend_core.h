#ifndef AL_RENDERER_BACKEND_CORE_H
#define AL_RENDERER_BACKEND_CORE_H

#include "engine/memory/memory.h"
#include "engine/platform/platform.h"

namespace al
{
    struct RendererBackendInitData
    {
        AllocatorBindings   bindings;
        const char*         applicationName;
        PlatformWindow*     window;
        bool                isDebug;
    };
}

#endif
