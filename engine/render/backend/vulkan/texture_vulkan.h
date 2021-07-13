#ifndef AL_IMAGE_2D_VULKAN_H
#define AL_IMAGE_2D_VULKAN_H

#include "../texture_base.h"
#include "vulkan_base.h"
#include "vulkan_memory_manager.h"

namespace al
{
    struct VulkanGpu;

    struct VulkanTexture : Texture
    {
        VulkanMemoryManager::Memory memory;
        VkImage handle;
        VkImageView view;
        VkFormat vkFormat;
        VkExtent3D extent;
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

    VkFormat vulkan_get_vk_format(TextureFormat format);
    bool vulkan_is_depth_image_format(TextureFormat format);
}

#endif
