
#include "framebuffer_vulkan.h"
#include "renderer_backend_vulkan.h"
#include "engine/utilities/utilities.h"

namespace al
{
    Framebuffer* vulkan_framebuffer_create(RendererBackend* _backend, FramebufferCreateInfo* createInfo)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        VulkanFramebuffer* framebuffer = data_block_storage_add(&backend->framebuffers);
        VulkanRenderStage* stage = (VulkanRenderStage*)createInfo->stage;
        Array<VkImageView> attachments;
        array_construct(&attachments, &backend->memoryManager.cpu_allocationBindings, createInfo->textures.size);
        defer(array_destruct(&attachments));
        for (auto it = create_iterator(&createInfo->textures); !is_finished(&it); advance(&it))
        {
            VulkanTexture* texture = (VulkanTexture*)*get(it);
            attachments[to_index(it)] = texture->view;
        }
        VkFramebufferCreateInfo framebufferInfo
        {
            .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .renderPass         = stage->renderPass,
            .attachmentCount    = u32(attachments.size),
            .pAttachments       = attachments.memory,
            .width              = createInfo->size.x,
            .height             = createInfo->size.y,
            .layers             = 1,
        };
        al_vk_check(vkCreateFramebuffer(backend->gpu.logicalHandle, &framebufferInfo, &backend->memoryManager.cpu_allocationCallbacks, &framebuffer->handle));
        return framebuffer;
    }

    void vulkan_framebuffer_destroy(RendererBackend* _backend, Framebuffer* _framebuffer)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        VulkanFramebuffer* framebuffer = (VulkanFramebuffer*)_framebuffer;
        vkDestroyFramebuffer(backend->gpu.logicalHandle, framebuffer->handle, &backend->memoryManager.cpu_allocationCallbacks);
    }
}
