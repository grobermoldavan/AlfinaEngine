#ifndef AL_FRAMEBUFFER_H
#define AL_FRAMEBUFFER_H

#include "engine/utilities/utilities.h"
#include "enums.h"

namespace al
{
    struct RenderDevice;
    struct RenderPass;
    struct Texture;

    struct Framebuffer
    {

    };

    struct FramebufferCreateInfo
    {
        PointerWithSize<Texture*> attachments;
        RenderPass* pass;
        RenderDevice* device;
    };
}

#endif
