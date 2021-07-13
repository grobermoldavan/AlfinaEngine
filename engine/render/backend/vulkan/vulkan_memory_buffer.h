#ifndef AL_VULKAN_MEMORY_BUFFER_H
#define AL_VULKAN_MEMORY_BUFFER_H

#include "engine/types.h"
#include "vulkan_base.h"
#include "vulkan_gpu.h"
#include "vulkan_memory_manager.h"

namespace al
{
    struct VulkanMemoryBuffer
    {
        VulkanMemoryManager::Memory memory;
        VkBuffer handle;
    };

    VulkanMemoryBuffer vulkan_memory_buffer_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, VkBufferCreateInfo* createInfo, VkMemoryPropertyFlags memoryProperty);
    VulkanMemoryBuffer vulkan_vertex_buffer_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes);
    VulkanMemoryBuffer vulkan_index_buffer_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes);
    VulkanMemoryBuffer vulkan_staging_buffer_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes);
    VulkanMemoryBuffer vulkan_uniform_buffer_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes);
    void vulkan_memory_buffer_destroy(VulkanMemoryBuffer* buffer, VulkanGpu* gpu, VulkanMemoryManager* memoryManager);

    void vulkan_copy_cpu_memory_to_buffer(VkDevice device, void* src, VulkanMemoryBuffer* dst, uSize sizeBytes);
    void vulkan_copy_buffer_to_buffer(VulkanGpu* gpu, VkCommandBuffer commandBuffer, VulkanMemoryBuffer* src, VulkanMemoryBuffer* dst, uSize sizeBytes, uSize srcOffsetBytes = 0, uSize dstOffsetBytes = 0);
    void vulkan_copy_buffer_to_image(VulkanGpu* gpu, VkCommandBuffer commandBuffer, VulkanMemoryBuffer* src, VkImage dst, VkExtent3D extent);
}

#endif
