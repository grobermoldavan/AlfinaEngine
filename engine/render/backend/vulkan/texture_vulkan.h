#ifndef AL_IMAGE_2D_VULKAN_H
#define AL_IMAGE_2D_VULKAN_H

#include "../texture_base.h"
#include "vulkan_base.h"
#include "vulkan_memory_manager.h"

namespace al
{
    struct VulkanGpu;
    struct VulkanRendererBackend;
    struct VulkanCommandBufferSet;

    struct VulkanTexture : Texture
    {
        VulkanMemoryManager::Memory memory;
        VkImage handle;
        VkImageView view;
        VkFormat vkFormat;
        VkExtent3D extent;
        VkImageLayout currentLayout;
        VkImageSubresourceRange subresourceRange;
    };

    struct VulkanTextureCreateInfo // for inner use
    {
        VulkanGpu* gpu;
        VulkanMemoryManager* memoryManager;
        VkExtent3D extent;
        VkFormat vkFormat;
        VkImageUsageFlags usage;
        VkImageAspectFlags aspect;
        VkImageType imageType;
        VkImageViewType viewType;
    };

    // Texture* vulkan_texture_create(RendererBackend* backend, const char* path);
    Texture* vulkan_texture_create(RendererBackend* backend, TextureCreateInfo* createInfo);
    void vulkan_texture_destroy(RendererBackend* backend, Texture* texture);
    u32_3 vulkan_texture_get_size(Texture* texture);

    void vulkan_texture_destroy_internal(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, VulkanTexture* texture);
    void vulkan_swap_chain_texture_construct(VulkanTexture* result, VkImageView view, VkExtent2D extent, VkFormat format, VkFormat depthStencilFormat);
    VkFormat vulkan_get_vk_format(TextureFormat format);
    bool vulkan_is_depth_image_format(TextureFormat format);
    void vulkan_transition_image_layout(VulkanGpu* gpu, VulkanCommandBufferSet* commandBufferSet, VkImage image, VkImageSubresourceRange* subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout);
}

#endif
