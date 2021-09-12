#ifndef AL_RENDER_COMMAND_BUFFER_H
#define AL_RENDER_COMMAND_BUFFER_H

#include "engine/types.h"
#include "enums.h"

namespace al
{
    struct RenderDevice;
    struct RenderPipeline;
    struct Framebuffer;

    struct CommandBuffer
    {

    };

    struct CommandBufferRequestInfo
    {
        RenderDevice* device;
        CommandBufferUsage usage;
    };

    struct CommandBindPipelineInfo
    {
        RenderPipeline* pipeline;
        Framebuffer* framebuffer;
    };

    struct CommandDrawInfo
    {
        u32 vertexCount;
    };
}

#endif
