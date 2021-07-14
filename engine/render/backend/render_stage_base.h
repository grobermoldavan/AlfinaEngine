#ifndef AL_RENDER_STAGE_BASE_H
#define AL_RENDER_STAGE_BASE_H

#include "engine/utilities/utilities.h"
#include "engine/platform/platform.h"
#include "texture_formats.h"

namespace al
{
    struct RendererBackend;
    struct FramebufferDescription;
    struct Framebuffer;

    struct ShaderProgram
    {

    };

    struct ShaderProgramVtable
    {
        ShaderProgram* (*create)(RendererBackend* backend, PlatformFile bytecode);
        void (*destroy)(RendererBackend* backend, ShaderProgram* program);
    };

    struct RenderStage
    {

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

    struct RenderStageCreateInfo
    {
        struct OutputAttachmentDescription
        {
            using FlagsT = u8;
            enum Flags : FlagsT
            {
                SWAP_CHAIN_ATTACHMENT = FlagsT(1) << 0,
            };
            TextureFormat format;
            FlagsT flags;
        };
        struct StencilOpState
        {
            u32         compareMask;
            u32         writeMask;
            u32         reference;
            StencilOp   failOp;
            StencilOp   passOp;
            StencilOp   depthFailOp;
            CompareOp   compareOp;
        };
        enum
        {
            GRAPHICS,
        } type;
        union
        {
            struct
            {
                StencilOpState stencilOpState;
                ShaderProgram* vertexShader;
                ShaderProgram* fragmentShader;
                FramebufferDescription* framebufferDescription;
            } graphics;
        };
    };

    struct RenderStageVtable
    {
        RenderStage* (*create)(RendererBackend* backend, RenderStageCreateInfo* createInfo);
        void (*destroy)(RendererBackend* backend, RenderStage* stage);
        void (*bind)(RendererBackend* backend, RenderStage* stage, Framebuffer* framebuffer);
    };
}

#endif
