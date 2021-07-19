
#include "vulkan_memory_buffer.h"
#include "vulkan_utils.h"

namespace al
{
    VulkanMemoryBuffer vulkan_memory_buffer_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, VkBufferCreateInfo* createInfo, VkMemoryPropertyFlags memoryProperty)
    {
        VulkanMemoryBuffer buffer{};
        al_vk_check(vkCreateBuffer(gpu->logicalHandle, createInfo, &memoryManager->cpu_allocationCallbacks, &buffer.handle));
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(gpu->logicalHandle, buffer.handle, &memoryRequirements);
        u32 memoryTypeIndex;
        utils::get_memory_type_index(&gpu->memoryProperties, memoryRequirements.memoryTypeBits, memoryProperty, &memoryTypeIndex);
        buffer.memory = gpu_allocate(memoryManager, gpu->logicalHandle, { .sizeBytes = memoryRequirements.size, .alignment = memoryRequirements.alignment, .memoryTypeIndex = memoryTypeIndex });
        al_vk_check(vkBindBufferMemory(gpu->logicalHandle, buffer.handle, buffer.memory.memory, buffer.memory.offsetBytes));
        return buffer;
    }

    VulkanMemoryBuffer vulkan_vertex_buffer_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes)
    {
        VkBufferCreateInfo bufferInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .size                   = sizeSytes,
            .usage                  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = 0,        // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
            .pQueueFamilyIndices    = nullptr,  // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
        };
        u32 queueFamilyIndices[] =
        {
            get_command_queue(gpu, VulkanGpu::CommandQueue::GRAPHICS)->queueFamilyIndex,
            get_command_queue(gpu, VulkanGpu::CommandQueue::TRANSFER)->queueFamilyIndex,
        };
        if (queueFamilyIndices[0] != queueFamilyIndices[1])
        {
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = 2;
            bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        return vulkan_memory_buffer_create(gpu, memoryManager, &bufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    VulkanMemoryBuffer vulkan_index_buffer_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes)
    {
        VkBufferCreateInfo bufferInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .size                   = sizeSytes,
            .usage                  = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = 0,        // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
            .pQueueFamilyIndices    = nullptr,  // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
        };
        u32 queueFamilyIndices[] =
        {
            get_command_queue(gpu, VulkanGpu::CommandQueue::GRAPHICS)->queueFamilyIndex,
            get_command_queue(gpu, VulkanGpu::CommandQueue::TRANSFER)->queueFamilyIndex,
        };
        if (queueFamilyIndices[0] != queueFamilyIndices[1])
        {
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = 2;
            bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        return vulkan_memory_buffer_create(gpu, memoryManager, &bufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    VulkanMemoryBuffer vulkan_staging_buffer_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes)
    {
        VkBufferCreateInfo bufferInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .size                   = sizeSytes,
            .usage                  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = 0,        // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
            .pQueueFamilyIndices    = nullptr,  // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
        };
        return vulkan_memory_buffer_create(gpu, memoryManager, &bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    VulkanMemoryBuffer vulkan_uniform_buffer_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes)
    {
        VkBufferCreateInfo bufferInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .size                   = sizeSytes,
            .usage                  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = 0,        // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
            .pQueueFamilyIndices    = nullptr,  // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
        };
        return vulkan_memory_buffer_create(gpu, memoryManager, &bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void vulkan_memory_buffer_destroy(VulkanMemoryBuffer* buffer, VulkanGpu* gpu, VulkanMemoryManager* memoryManager)
    {
        gpu_deallocate(memoryManager, gpu->logicalHandle, buffer->memory);
        vkDestroyBuffer(gpu->logicalHandle, buffer->handle, &memoryManager->cpu_allocationCallbacks);
        std::memset(&buffer, 0, sizeof(VulkanMemoryBuffer));
    }

    void vulkan_copy_cpu_memory_to_buffer(VkDevice device, void* src, VulkanMemoryBuffer* dst, uSize sizeBytes)
    {
        void* mappedMemory;
        vkMapMemory(device, dst->memory.memory, dst->memory.offsetBytes, sizeBytes, 0, &mappedMemory);
        std::memcpy(mappedMemory, src, sizeBytes);
        vkUnmapMemory(device, dst->memory.memory);
    }

    void vulkan_copy_buffer_to_buffer(VulkanGpu* gpu, VkCommandBuffer commandBuffer, VulkanMemoryBuffer* src, VulkanMemoryBuffer* dst, uSize sizeBytes, uSize srcOffsetBytes, uSize dstOffsetBytes)
    {
        VkCommandBufferBeginInfo beginInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext              = nullptr,
            .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo   = nullptr,
        };
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        VkBufferCopy copyRegion
        {
            .srcOffset  = srcOffsetBytes,
            .dstOffset  = dstOffsetBytes,
            .size       = sizeBytes,
        };
        vkCmdCopyBuffer(commandBuffer, src->handle, dst->handle, 1, &copyRegion);
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
        VkQueue queue = get_command_queue(gpu, VulkanGpu::CommandQueue::TRANSFER)->handle;
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);
    }

    void vulkan_copy_buffer_to_image(VulkanGpu* gpu, VkCommandBuffer commandBuffer, VulkanMemoryBuffer* src, VkImage dst, VkExtent3D extent)
    {
        // VkCommandBufferBeginInfo beginInfo
        // {
        //     .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        //     .pNext              = nullptr,
        //     .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        //     .pInheritanceInfo   = nullptr,
        // };
        // vkBeginCommandBuffer(commandBuffer, &beginInfo);
        VkBufferImageCopy imageCopy
        {
            .bufferOffset       = 0,
            .bufferRowLength    = 0,
            .bufferImageHeight  = 0,
            .imageSubresource   =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,    // @TODO : unhardcode this
                .mipLevel       = 0,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            .imageOffset        = 0,
            .imageExtent        = extent,
        };
        vkCmdCopyBufferToImage(commandBuffer, src->handle, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
        // vkEndCommandBuffer(commandBuffer);
        // VkSubmitInfo submitInfo
        // {
        //     .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        //     .pNext                  = nullptr,
        //     .waitSemaphoreCount     = 0,
        //     .pWaitSemaphores        = nullptr,
        //     .pWaitDstStageMask      = 0,
        //     .commandBufferCount     = 1,
        //     .pCommandBuffers        = &commandBuffer,
        //     .signalSemaphoreCount   = 0,
        //     .pSignalSemaphores      = nullptr,
        // };
        // VkQueue queue = get_command_queue(gpu, VulkanGpu::CommandQueue::TRANSFER)->handle;
        // vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        // vkQueueWaitIdle(queue);
    }
}
