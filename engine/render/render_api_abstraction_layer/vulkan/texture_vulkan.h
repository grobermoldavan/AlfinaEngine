#ifndef AL_TEXTURE_VULKAN_H
#define AL_TEXTURE_VULKAN_H

#include "../base/texture.h"
#include "vulkan_base.h"

namespace al
{
    struct RenderDeviceVulkan;

    struct TextureVulkan : Texture
    {
        using FlagsT = u64;
        enum Flags
        {
            SWAP_CHAIN_TEXTURE = FlagsT(1) << 0,
        };
        VkImageSubresourceRange subresourceRange;
        VkExtent3D extent;
        RenderDeviceVulkan* device;
        VkImage image;
        VkImageView imageView;
        FlagsT flags;
        VkFormat format;
        VkImageLayout currentLayout;
        VkSampler defaultSampler; // todo
    };

    Texture* vulkan_texture_create(TextureCreateInfo* createInfo);
    void vulkan_texture_destroy(Texture* texture);
}

#endif
