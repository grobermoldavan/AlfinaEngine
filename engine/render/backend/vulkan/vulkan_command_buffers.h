#ifndef AL_VULKAN_COMMAND_BUFFER_H
#define AL_VULKAN_COMMAND_BUFFER_H

#include "vulkan_base.h"
#include "engine/utilities/utilities.h"

namespace al
{
    struct VulkanGpu;
    struct VulkanMemoryManager;

    struct VulkanCommandPool
    {
        VkCommandPool handle;
        VkQueueFlags queueFlags;
    };

    struct VulkanCommandBuffer
    {
        VkCommandBuffer handle;
        VkQueueFlags queueFlags;
    };

    struct VulkanCommandPoolSet
    {
        Array<VulkanCommandPool> pools;
    };

    struct VulkanCommandBufferSet
    {
        Array<VulkanCommandBuffer> buffers;
    };

    VulkanCommandPoolSet vulkan_command_pool_set_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager);
    VulkanCommandBufferSet vulkan_command_buffer_set_create(VulkanCommandPoolSet* poolSet, VulkanGpu* gpu, VulkanMemoryManager* memoryManager);
    void vulkan_command_pool_set_destroy(VulkanCommandPoolSet* set, VulkanGpu* gpu, VulkanMemoryManager* memoryManager);
    void vulkan_command_buffer_set_destroy(VulkanCommandBufferSet* set, VulkanGpu* gpu, VulkanMemoryManager* memoryManager);
    VulkanCommandPool* vulkan_get_command_pool(VulkanCommandPoolSet* set, VkQueueFlags flags);
    VulkanCommandBuffer* vulkan_get_command_buffer(VulkanCommandBufferSet* set, VkQueueFlags flags);
}

#endif
