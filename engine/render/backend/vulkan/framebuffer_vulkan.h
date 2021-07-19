#ifndef AL_FRAMEBUFFER_VULKAN_H
#define AL_FRAMEBUFFER_VULKAN_H

#include "../framebuffer_base.h"
#include "vulkan_base.h"
#include "engine/utilities/utilities.h"

namespace al
{
    struct RendererBackend;
    struct VulkanTexture;

    struct VulkanFramebuffer : Framebuffer
    {
        VkFramebuffer handle;
        Array<VulkanTexture*> textures;
    };

    Framebuffer* vulkan_framebuffer_create(RendererBackend* backend, FramebufferCreateInfo* createInfo);
    void vulkan_framebuffer_destroy(RendererBackend* backend, Framebuffer* framebuffer);
}

#endif
