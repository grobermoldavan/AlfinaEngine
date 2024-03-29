#ifndef AL_RENDER_PASS_VULKAN_H
#define AL_RENDER_PASS_VULKAN_H

#include "../base/render_pass.h"
#include "vulkan_base.h"

namespace al
{
    struct RenderDeviceVulkan;

    using RenderPassAttachmentRefBitMask = u32;

    struct VulkanSubpassInfo
    {
        RenderPassAttachmentRefBitMask inputAttachmentRefs;
        RenderPassAttachmentRefBitMask colorAttachmentRefs;
    };

    struct VulkanRenderPassAttachmentInfo
    {
        VkImageLayout initialLayout;
        VkImageLayout finalLayout;
        VkFormat format;
    };

    struct RenderPassVulkan : RenderPass
    {
        RenderDeviceVulkan* device;
        VkRenderPass handle;
        Array<VulkanSubpassInfo> subpassInfos;
        Array<VulkanRenderPassAttachmentInfo> attachmentInfos;
        Array<VkClearValue> clearValues;
    };

    RenderPass* vulkan_render_pass_create(RenderPassCreateInfo* createInfo);
    void vulkan_render_pass_destroy(RenderPassVulkan* pass);
}

#endif
