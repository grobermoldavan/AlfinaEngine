
#include "texture_vulkan.h"
#include "render_device_vulkan.h"

namespace al
{
    Texture* vulkan_texture_create(TextureCreateInfo* createInfo)
    {
        RenderDeviceVulkan* device = (RenderDeviceVulkan*)createInfo->device;
        TextureVulkan* texture = allocate<TextureVulkan>(&device->memoryManager.cpu_persistentAllocator);
        return texture;
    }

    void vulkan_texture_destroy(Texture* _texture)
    {
        TextureVulkan* texture = (TextureVulkan*)_texture;
        RenderDeviceVulkan* device = texture->device;
        if (texture->flags & TextureVulkan::SWAP_CHAIN_TEXTURE)
        {
            // Swap chain textures are destroyed by device
            return;
        }
        deallocate<TextureVulkan>(&device->memoryManager.cpu_persistentAllocator, texture);
    }
}
