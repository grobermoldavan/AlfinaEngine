#ifndef AL_FRAMEBUFFER_VULKAN_H
#define AL_FRAMEBUFFER_VULKAN_H

#include "../base/framebuffer.h"
#include "vulkan_base.h"

namespace al
{
    struct RenderDeviceVulkan;
    struct TextureVulkan;
    struct RenderPassVulkan;

    struct FramebufferVulkan : Framebuffer
    {
        Array<TextureVulkan*> textures;
        RenderDeviceVulkan* device;
        RenderPassVulkan* renderPass;
        VkFramebuffer handle;
    };

    Framebuffer* vulkan_framebuffer_create(FramebufferCreateInfo* createInfo);
    void vulkan_framebuffer_destroy(Framebuffer* framebuffer);

    void vulkan_framebuffer_prepare(FramebufferVulkan* framebuffer);
}

#endif
