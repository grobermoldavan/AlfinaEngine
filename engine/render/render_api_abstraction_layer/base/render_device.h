#ifndef AL_RENDER_DEVICE_H
#define AL_RENDER_DEVICE_H

#include "engine/types.h"

namespace al
{
    struct PlatformWindow;
    struct AllocatorBindings;

    struct RenderDevice
    {
        
    };

    struct RenderDeviceCreateInfo
    {
        using FlagsT = u64;
        enum Flags : FlagsT
        {
            IS_DEBUG = FlagsT(1) << 0,
        };
        FlagsT flags;
        PlatformWindow* window;
        AllocatorBindings* persistentAllocator;
        AllocatorBindings* frameAllocator;
    };
}

#endif
