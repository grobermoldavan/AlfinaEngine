#ifndef AL_FRAMEBUFFER_BASE_H
#define AL_FRAMEBUFFER_BASE_H

#include "engine/types.h"
#include "engine/math/math.h"
#include "engine/utilities/utilities.h"
#include "texture_formats.h"

namespace al
{
    struct RendererBackend;
    struct Texture;
    struct RenderStage;

    struct Framebuffer
    {

    };

    struct FramebufferDescription
    {
        struct AttachmentDescription
        {
            using FlagsT = u8;
            enum Flags : FlagsT
            {
                SWAP_CHAIN_ATTACHMENT = FlagsT(1) << 0,
            };
            TextureFormat format;
            FlagsT flags;
        };
        PointerWithSize<AttachmentDescription> attachmentDescriptions;
    };

    struct FramebufferCreateInfo
    {
        RenderStage* stage;
        PointerWithSize<Texture*> textures;
        u32_2 size;
    };

    struct FramebufferVtable
    {
        Framebuffer* (*create)(RendererBackend* backend, FramebufferCreateInfo* createInfo);
        void (*destroy)(RendererBackend* backend, Framebuffer* framebuffer);
    };
}

#endif
