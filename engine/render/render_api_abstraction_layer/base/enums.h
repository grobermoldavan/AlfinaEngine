#ifndef AL_TEXTURE_FORMATS_H
#define AL_TEXTURE_FORMATS_H

#include "engine/types.h"

namespace al
{
    enum struct TextureFormat : u8
    {
        DEPTH_STENCIL,
        SWAP_CHAIN,
        RGBA_8,
        RGBA_32F,
    };

    enum struct AttachmentLoadOp : u8
    {
        NOTHING,
        CLEAR,
        LOAD,
    };

    enum struct AttachmentStoreOp : u8
    {
        NOTHING,
        STORE,
    };

    enum struct PipelinePoligonMode : u8
    {
        FILL,
        LINE,
        POINT,
    };

    enum struct PipelineCullMode : u8
    {
        NONE,
        FRONT,
        BACK,
        FRONT_BACK,
    };

    enum struct PipelineFrontFace : u8
    {
        CLOCKWISE,
        COUNTER_CLOCKWISE,
    };

    enum struct MultisamplingType : u8
    {
        SAMPLE_1,
        SAMPLE_2,
        SAMPLE_4,
        SAMPLE_8,
    };

    enum struct StencilOp : u8
    {
        KEEP,
        ZERO,
        REPLACE,
        INCREMENT_AND_CLAMP,
        DECREMENT_AND_CLAMP,
        INVERT,
        INCREMENT_AND_WRAP,
        DECREMENT_AND_WRAP,
    };

    enum struct CompareOp : u8
    {
        NEVER,
        LESS,
        EQUAL,
        LESS_OR_EQUAL,
        GREATER,
        NOT_EQUAL,
        GREATER_OR_EQUAL,
        ALWAYS,
    };

    enum struct CommandBufferUsage : u8
    {
        GRAPHICS,
        COMPUTE,
        TRANSFER,
    };

    enum struct ProgramStage : u32
    {
        Vertex   = u32(1) << 0,
        Fragment = u32(1) << 1,
        Compute  = u32(1) << 2,
    };
    using ProgramStageFlags = u32;

    enum struct MemoryBufferUsage : u32
    {
        TransferSrc     = u32(1) << 0,
        TransferDst     = u32(1) << 1,
        UniformTexel    = u32(1) << 2,
        StorageTexel    = u32(1) << 3,
        UniformBuffer   = u32(1) << 4,
        StorageBuffer   = u32(1) << 5,
        IndexBuffer     = u32(1) << 6,
        VertexBuffer    = u32(1) << 7,
        IndirectBuffer  = u32(1) << 8,
    };
    using MemoryBufferUsageFlags = u32;
}

#endif
