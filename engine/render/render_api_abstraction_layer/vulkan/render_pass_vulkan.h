#ifndef AL_RENDER_PASS_VULKAN_H
#define AL_RENDER_PASS_VULKAN_H

#include "../base/render_pass.h"
#include "vulkan_base.h"

namespace al
{
    struct RenderDeviceVulkan;

    struct RenderPassVulkan : RenderPass
    {
        RenderDeviceVulkan* device;
        VkRenderPass renderPass;
    };

    RenderPass* vulkan_render_pass_create(RenderPassCreateInfo* createInfo);
    void vulkan_render_pass_destroy(RenderPassVulkan* pass);
}

#endif
