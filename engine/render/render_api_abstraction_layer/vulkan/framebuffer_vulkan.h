#ifndef AL_FRAMEBUFFER_VULKAN_H
#define AL_FRAMEBUFFER_VULKAN_H

#include "../base/framebuffer.h"
#include "vulkan_base.h"

namespace al
{
    struct RenderDeviceVulkan;
    struct TextureVulkan;

    struct FramebufferVulkan : Framebuffer
    {
        RenderDeviceVulkan* device;
        VkFramebuffer handle;
        // Array<TextureVulkan*> textures;
    };

    Framebuffer* vulkan_framebuffer_create(FramebufferCreateInfo* createInfo);
    void vulkan_framebuffer_destroy(Framebuffer* framebuffer);
}

#endif
