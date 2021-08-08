#ifndef AL_RENDER_PROGRAM_H
#define AL_RENDER_PROGRAM_H

#include "engine/types.h"

namespace al
{
    struct RenderDevice;

    struct RenderProgram
    {

    };

    struct RenderProgramCreateInfo
    {
        RenderDevice* device;
        u32* bytecode;
        uSize codeSizeBytes;
    };
}

#endif
