
#include "framebuffer_vulkan.h"
#include "render_pass_vulkan.h"
#include "render_device_vulkan.h"
#include "command_buffer_vulkan.h"

namespace al
{
    Framebuffer* vulkan_framebuffer_create(FramebufferCreateInfo* createInfo)
    {
        RenderDeviceVulkan* device = (RenderDeviceVulkan*)createInfo->device;
        RenderPassVulkan* pass = (RenderPassVulkan*)createInfo->pass;
        FramebufferVulkan* framebuffer = allocate<FramebufferVulkan>(&device->memoryManager.cpu_persistentAllocator);
        //
        // Validation
        //
        al_assert(pass->attachmentInfos.size == createInfo->attachments.size);
        for (al_iterator(it, createInfo->attachments))
        {
            TextureVulkan* texture = (TextureVulkan*)*get(it);
            al_assert(texture->format == pass->attachmentInfos[to_index(it)].format);
        }
        //
        // Temp data
        //
        Array<VkImageView> attachmentViews;
        array_construct(&attachmentViews, &device->memoryManager.cpu_frameAllocator, createInfo->attachments.size);
        defer(array_destruct(&attachmentViews));
        for (al_iterator(it, createInfo->attachments))
        {
            TextureVulkan* texture = (TextureVulkan*)*get(it);
            attachmentViews[to_index(it)] = texture->imageView;
        }
        //
        // Create framebuffer
        //
        VkFramebufferCreateInfo framebufferCreateInfo
        {
            .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .renderPass         = pass->handle,
            .attachmentCount    = u32(attachmentViews.size),
            .pAttachments       = attachmentViews.memory,
            .width              = ((TextureVulkan*)createInfo->attachments[0])->extent.width,
            .height             = ((TextureVulkan*)createInfo->attachments[0])->extent.height,
            .layers             = 1,
        };
        al_vk_check(vkCreateFramebuffer(device->gpu.logicalHandle, &framebufferCreateInfo, &device->memoryManager.cpu_allocationCallbacks, &framebuffer->handle));
        array_construct(&framebuffer->textures, &device->memoryManager.cpu_persistentAllocator, createInfo->attachments.size);
        for (al_iterator(it, createInfo->attachments))
        {
            TextureVulkan* texture = (TextureVulkan*)*get(it);
            framebuffer->textures[to_index(it)] = texture;
        }
        framebuffer->device = device;
        framebuffer->renderPass = pass;
        return framebuffer;
    }

    void vulkan_framebuffer_destroy(Framebuffer* _framebuffer)
    {
        FramebufferVulkan* framebuffer = (FramebufferVulkan*)_framebuffer;
        array_destruct(&framebuffer->textures);
        vkDestroyFramebuffer(framebuffer->device->gpu.logicalHandle, framebuffer->handle, &framebuffer->device->memoryManager.cpu_allocationCallbacks);
        deallocate<FramebufferVulkan>(&framebuffer->device->memoryManager.cpu_persistentAllocator, framebuffer);
    }

    void vulkan_framebuffer_prepare(FramebufferVulkan* framebuffer)
    {
        RenderDeviceVulkan* device = framebuffer->device;
        VulkanGpu::CommandQueue* transferQueue = get_command_queue(&device->gpu, VulkanGpu::CommandQueue::Flags::TRANSFER);
        CommandBufferVulkan* commandBuffer = nullptr;
        for (al_iterator(it, framebuffer->textures))
        {
            TextureVulkan* texture = *get(it);
            VkImageLayout initialLayout = framebuffer->renderPass->attachmentInfos[to_index(it)].initialLayout;
            if (texture->currentLayout != initialLayout)
            {
                if (!commandBuffer)
                {
                    CommandBufferRequestInfo requestInfo { device, CommandBufferUsage::TRANSFER };
                    commandBuffer = (CommandBufferVulkan*)vulkan_command_buffer_request(&requestInfo);
                }
                vulkan_command_buffer_transition_image_layout(commandBuffer, texture, initialLayout);
            }
        }
        if (commandBuffer)
        {
            vulkan_command_buffer_submit(commandBuffer);
        }
    }
}
