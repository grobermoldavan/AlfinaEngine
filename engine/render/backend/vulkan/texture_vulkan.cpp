
// //
// // @TODO : use custom allocator for stbi
// //
// #define STB_IMAGE_IMPLEMENTATION
// #include "engine/3d_party_libs/stb/stb_image.h"

#include "texture_vulkan.h"
#include "vulkan_utils.h"
#include "vulkan_utils_converters.h"
#include "vulkan_memory_buffer.h"
#include "renderer_backend_vulkan.h"

namespace al
{
    void vulkan_fill_image_attachment_sharing_mode(VkSharingMode* resultMode, u32* resultCount, PointerWithSize<u32> resultFamilyIndices, VulkanGpu* gpu);
    void vulkan_construct_image_attachment(VulkanTexture* texture, VulkanTextureCreateInfo* createInfo);
    VkImageUsageFlags vulkan_get_image_usage(TextureFormat format);
    VkImageAspectFlags vulkan_get_image_aspect(TextureFormat format, bool hasStencil);
    void vulkan_copy_data_to_image(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, VkCommandBuffer commandBuffer, void* data, VulkanTexture* texture, uSize dataSize);
    void vulkan_transition_image_layout(VulkanGpu* gpu, VkCommandBuffer commandBuffer, VkImage image, VkFormat vkFormat, VkImageLayout oldLayout, VkImageLayout newLayout);

    // Texture* texture_create(RendererBackend* _backend, const char* path)
    // {
    //     VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
    //     s32 stbi_width;
    //     s32 stbi_height;
    //     s32 stbi_channels;
    //     stbi_uc* data = stbi_load(path, &stbi_width, &stbi_height, &stbi_channels, 0);
    //     al_vk_assert(data && "Unable to load image");
    //     Texture* result = data_block_storage_add(&backend->textures);
    //     VulkanTextureCreateInfo innerCreateInfo
    //     {
    //         .gpu            = &backend->gpu,
    //         .memoryManager  = &backend->memoryManager,
    //         .extent         = { .width = u32(stbi_width), .height = u32(stbi_height), },
    //         .format         = {},
    //         .usage          = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    //         .aspect         = VK_IMAGE_ASPECT_COLOR_BIT,
    //     };
    //     switch(stbi_channels)
    //     {
    //         case 4: innerCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB; break;
    //         default: al_vk_assert(!"Unsupported number of channels");
    //     }
    //     vulkan_construct_image_attachment(result, &innerCreateInfo);
    //     vulkan_copy_data_to_image(&backend->gpu, &backend->memoryManager, backend->transferBuffer, data, result, stbi_width * stbi_height * stbi_channels);
    //     stbi_image_free(data);
    //     return result;
    // }

    Texture* vulkan_texture_create(RendererBackend* _backend, TextureCreateInfo* createInfo)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        VulkanTexture* result = data_block_storage_add(&backend->textures);
        VulkanTextureCreateInfo innerCreateInfo;
        if (createInfo->type == TextureType::_2D)
        {
            VkFormat vkFormat = createInfo->format == TextureFormat::DEPTH_STENCIL ? backend->gpu.depthStencilFormat : utils::converters::to_vk_format(createInfo->format);
            innerCreateInfo =
            {
                .gpu            = &backend->gpu,
                .memoryManager  = &backend->memoryManager,
                .extent         = utils::converters::to_extent(createInfo->size),
                .vkFormat       = vkFormat,
                .usage          = vulkan_get_image_usage(createInfo->format),
                .aspect         = vulkan_get_image_aspect(createInfo->format, backend->gpu.flags & VulkanGpu::HAS_STENCIL),
                .imageType      = VK_IMAGE_TYPE_2D,
                .viewType       = VK_IMAGE_VIEW_TYPE_2D,
            };
        }
        vulkan_construct_image_attachment(result, &innerCreateInfo);
        al_vk_memcpy((void*)&result->size, &createInfo->size, sizeof(createInfo->size));
        al_vk_memcpy((void*)&result->type, &createInfo->type, sizeof(createInfo->type));
        al_vk_memcpy((void*)&result->format, &createInfo->format, sizeof(createInfo->format));
        return (Texture*)result;
    }

    void vulkan_texture_destroy(RendererBackend* _backend, Texture* _texture)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        VulkanTexture* texture = (VulkanTexture*)_texture;
        vulkan_texture_destroy_internal(&backend->gpu, &backend->memoryManager, texture);
    }

    void vulkan_texture_destroy_internal(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, VulkanTexture* texture)
    {
        vkDestroyImageView(gpu->logicalHandle, texture->view, &memoryManager->cpu_allocationCallbacks);
        if (texture->handle) vkDestroyImage(gpu->logicalHandle, texture->handle, &memoryManager->cpu_allocationCallbacks);
        if (gpu_is_valid_memory(texture->memory)) gpu_deallocate(memoryManager, gpu->logicalHandle, texture->memory);
    }

    //
    // This function is used by vulkan renderer backend internally to create texture handles to swap chain images
    //
    void vulkan_swap_chain_texture_construct(VulkanTexture* result, VkImageView view, VkExtent2D extent, VkFormat format, VkFormat depthStencilFormat)
    {
        const u32_3 textureSize = { extent.width, extent.height, 1 };
        const TextureType textureType = TextureType::_2D;
        const TextureFormat textureFormat = format == depthStencilFormat ? TextureFormat::DEPTH_STENCIL : utils::converters::to_texture_format(format);
        result->memory     = {};
        result->handle     = VK_NULL_HANDLE;
        result->view       = view;
        result->vkFormat   = format;
        result->extent     = utils::converters::to_extent(textureSize);
        al_vk_memcpy((void*)&result->size, &textureSize, sizeof(result->size));
        al_vk_memcpy((void*)&result->type, &textureType, sizeof(result->type));
        al_vk_memcpy((void*)&result->format, &textureFormat, sizeof(result->format));
    }

    void vulkan_fill_image_attachment_sharing_mode(VkSharingMode* resultMode, u32* resultCount, PointerWithSize<u32> resultFamilyIndices, VulkanGpu* gpu)
    {
        al_vk_assert(resultFamilyIndices.size >= 2);
        VulkanGpu::CommandQueue* graphicsQueue = get_command_queue(gpu, VulkanGpu::CommandQueue::GRAPHICS);
        VulkanGpu::CommandQueue* transferQueue = get_command_queue(gpu, VulkanGpu::CommandQueue::TRANSFER);
        if (graphicsQueue->queueFamilyIndex == transferQueue->queueFamilyIndex)
        {
            *resultMode = VK_SHARING_MODE_EXCLUSIVE;
            *resultCount = 0;
        }
        else
        {
            *resultMode = VK_SHARING_MODE_CONCURRENT;
            *resultCount = 2;
            resultFamilyIndices[0] = graphicsQueue->queueFamilyIndex;
            resultFamilyIndices[1] = transferQueue->queueFamilyIndex;
        }
    }

    void vulkan_construct_image_attachment(VulkanTexture* texture, VulkanTextureCreateInfo* createInfo)
    {
        texture->vkFormat = createInfo->vkFormat;
        {
            u32 queueFamiliesIndices[VulkanGpu::MAX_UNIQUE_COMMAND_QUEUES];
            VkImageCreateInfo vkImageCreateInfo
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .imageType              = createInfo->imageType,
                .format                 = createInfo->vkFormat,
                .extent                 = createInfo->extent,
                .mipLevels              = 1,
                .arrayLayers            = 1,
                .samples                = VK_SAMPLE_COUNT_1_BIT,
                .tiling                 = VK_IMAGE_TILING_OPTIMAL,
                .usage                  = createInfo->usage,
                .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount  = 0,
                .pQueueFamilyIndices    = queueFamiliesIndices,
                .initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            vulkan_fill_image_attachment_sharing_mode(&vkImageCreateInfo.sharingMode, &vkImageCreateInfo.queueFamilyIndexCount, { .ptr = queueFamiliesIndices, .size = array_size(queueFamiliesIndices) }, createInfo->gpu);
            al_vk_check(vkCreateImage(createInfo->gpu->logicalHandle, &vkImageCreateInfo, &createInfo->memoryManager->cpu_allocationCallbacks, &texture->handle));
        }
        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(createInfo->gpu->logicalHandle, texture->handle, &memoryRequirements);
        u32 memoryTypeIndex;
        bool result = utils::get_memory_type_index(&createInfo->gpu->memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex);
        al_vk_assert(result);
        texture->memory = gpu_allocate(createInfo->memoryManager, createInfo->gpu->logicalHandle, { .sizeBytes = memoryRequirements.size, .alignment = memoryRequirements.alignment, .memoryTypeIndex = memoryTypeIndex });
        al_vk_check(vkBindImageMemory(createInfo->gpu->logicalHandle, texture->handle, texture->memory.memory, texture->memory.offsetBytes));
        VkImageViewCreateInfo vkImageViewCreateInfo
        {
            .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .image              = texture->handle,
            .viewType           = createInfo->viewType,
            .format             = createInfo->vkFormat,
            .components         = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange   =
            {
                .aspectMask     = createInfo->aspect,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        al_vk_check(vkCreateImageView(createInfo->gpu->logicalHandle, &vkImageViewCreateInfo, &createInfo->memoryManager->cpu_allocationCallbacks, &texture->view));
    }

    VkImageUsageFlags vulkan_get_image_usage(TextureFormat format)
    {
        return format == TextureFormat::DEPTH_STENCIL ?
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    VkImageAspectFlags vulkan_get_image_aspect(TextureFormat format, bool hasStencil)
    {
        return format == TextureFormat::DEPTH_STENCIL ?
                VK_IMAGE_ASPECT_DEPTH_BIT | (hasStencil ? VK_IMAGE_ASPECT_STENCIL_BIT : 0) :
                VK_IMAGE_ASPECT_COLOR_BIT;
    }

    void vulkan_copy_data_to_image(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, VkCommandBuffer commandBuffer, void* data, VulkanTexture* texture, uSize dataSize)
    {
        vulkan_transition_image_layout(gpu, commandBuffer, texture->handle, texture->vkFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VulkanMemoryBuffer stagingBuffer = vulkan_staging_buffer_create(gpu, memoryManager, dataSize);
        defer(vulkan_memory_buffer_destroy(&stagingBuffer, gpu, memoryManager));
        vulkan_copy_cpu_memory_to_buffer(gpu->logicalHandle, data, &stagingBuffer, dataSize);
        vulkan_copy_buffer_to_image(gpu, commandBuffer, &stagingBuffer, texture->handle, utils::converters::to_extent(texture->size));
        vulkan_transition_image_layout(gpu, commandBuffer, texture->handle, texture->vkFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void vulkan_transition_image_layout(VulkanGpu* gpu, VkCommandBuffer commandBuffer, VkImage image, VkFormat vkFormat, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBufferBeginInfo beginInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext              = nullptr,
            .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo   = nullptr,
        };
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        VkImageMemoryBarrier barrier
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext                  = nullptr,
            .srcAccessMask          = 0,
            .dstAccessMask          = 0,
            .oldLayout              = oldLayout,
            .newLayout              = newLayout,
            .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .image                  = image,
            .subresourceRange       =
            {
                .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT, // @TODO : unhardcode this
                .baseMipLevel    = 0,
                .levelCount      = 1,
                .baseArrayLayer  = 0,
                .layerCount      = 1,
            },
        };
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask   = 0;
            barrier.dstAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage             = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage        = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
            sourceStage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            al_vk_assert(!"Unsupported image transition");
        }
        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        vkEndCommandBuffer(commandBuffer);
        VkSubmitInfo submitInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                  = nullptr,
            .waitSemaphoreCount     = 0,
            .pWaitSemaphores        = nullptr,
            .pWaitDstStageMask      = 0,
            .commandBufferCount     = 1,
            .pCommandBuffers        = &commandBuffer,
            .signalSemaphoreCount   = 0,
            .pSignalSemaphores      = nullptr,
        };
        VkQueue queue = get_command_queue(gpu, VulkanGpu::CommandQueue::GRAPHICS)->handle;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
    }
}
