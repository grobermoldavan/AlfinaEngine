#ifndef AL_FRAMEBUFFER_VULKAN_H
#define AL_FRAMEBUFFER_VULKAN_H

#include "../framebuffer_base.h"
#include "vulkan_base.h"

namespace al
{
    struct RendererBackend;

    struct VulkanFramebuffer : Framebuffer
    {
        VkFramebuffer handle;
    };

    Framebuffer* vulkan_framebuffer_create(RendererBackend* backend, FramebufferCreateInfo* createInfo);
    void vulkan_framebuffer_destroy(RendererBackend* backend, Framebuffer* framebuffer);
}

#endif
