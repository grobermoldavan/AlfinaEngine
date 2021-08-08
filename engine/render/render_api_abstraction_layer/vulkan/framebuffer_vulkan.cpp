
#include "framebuffer_vulkan.h"
#include "render_pass_vulkan.h"
#include "render_device_vulkan.h"

namespace al
{
    Framebuffer* vulkan_framebuffer_create(FramebufferCreateInfo* createInfo)
    {
        RenderDeviceVulkan* device = (RenderDeviceVulkan*)createInfo->device;
        RenderPassVulkan* pass = (RenderPassVulkan*)createInfo->pass;
        FramebufferVulkan* framebuffer = allocate<FramebufferVulkan>(&device->memoryManager.cpu_persistentAllocator);
        Array<VkImageView> attachmentViews;
        array_construct(&attachmentViews, &device->memoryManager.cpu_frameAllocator, createInfo->attachments.size);
        defer(array_destruct(&attachmentViews));
        for (al_iterator(it, createInfo->attachments))
        {
            TextureVulkan* texture = (TextureVulkan*)*get(it);
            attachmentViews[to_index(it)] = texture->imageView;
        }
        VkFramebufferCreateInfo framebufferCreateInfo
        {
            .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .renderPass         = pass->renderPass,
            .attachmentCount    = u32(attachmentViews.size),
            .pAttachments       = attachmentViews.memory,
            .width              = 20,
            .height             = 20,
            .layers             = 1,
        };
        al_vk_check(vkCreateFramebuffer(device->gpu.logicalHandle, &framebufferCreateInfo, &device->memoryManager.cpu_allocationCallbacks, &framebuffer->handle));
        framebuffer->device = device;
        return framebuffer;
    }

    void vulkan_framebuffer_destroy(Framebuffer* _framebuffer)
    {
        FramebufferVulkan* framebuffer = (FramebufferVulkan*)_framebuffer;
        vkDestroyFramebuffer(framebuffer->device->gpu.logicalHandle, framebuffer->handle, &framebuffer->device->memoryManager.cpu_allocationCallbacks);
        deallocate<FramebufferVulkan>(&framebuffer->device->memoryManager.cpu_persistentAllocator, framebuffer);
    }
}
