
#include "vulkan_command_buffers.h"
#include "vulkan_gpu.h"
#include "vulkan_memory_manager.h"

namespace al
{
    VulkanCommandPoolSet vulkan_command_pool_set_create(VulkanGpu* gpu, VulkanMemoryManager* memoryManager)
    {
        constexpr VkCommandPoolCreateFlags POOL_FLAGS = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        VulkanCommandPoolSet set;
        VulkanGpu::CommandQueue* graphicsQueue = get_command_queue(gpu, VulkanGpu::CommandQueue::GRAPHICS);
        VulkanGpu::CommandQueue* transferQueue = get_command_queue(gpu, VulkanGpu::CommandQueue::TRANSFER);
        if (graphicsQueue->queueFamilyIndex == transferQueue->queueFamilyIndex)
        {
            array_construct(&set.pools, &memoryManager->cpu_allocationBindings, 1);
            set.pools[0] = 
            {
                .handle = utils::create_command_pool(gpu->logicalHandle, graphicsQueue->queueFamilyIndex, &memoryManager->cpu_allocationCallbacks, POOL_FLAGS),
                .queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT,
            };
        }
        else
        {
            array_construct(&set.pools, &memoryManager->cpu_allocationBindings, 2);
            set.pools[0] =
            {
                .handle = utils::create_command_pool(gpu->logicalHandle, graphicsQueue->queueFamilyIndex, &memoryManager->cpu_allocationCallbacks, POOL_FLAGS),
                .queueFlags = VK_QUEUE_GRAPHICS_BIT,
            };
            set.pools[1] =
            {
                .handle = utils::create_command_pool(gpu->logicalHandle, transferQueue->queueFamilyIndex, &memoryManager->cpu_allocationCallbacks, POOL_FLAGS),
                .queueFlags = VK_QUEUE_TRANSFER_BIT,
            };
        }
        return set;
    }

    VulkanCommandBufferSet vulkan_command_buffer_set_create(VulkanCommandPoolSet* poolSet, VulkanGpu* gpu, VulkanMemoryManager* memoryManager)
    {
        VulkanCommandBufferSet set;
        VulkanCommandPool* graphicsPool = vulkan_get_command_pool(poolSet, VK_QUEUE_GRAPHICS_BIT);
        VulkanCommandPool* transferPool = vulkan_get_command_pool(poolSet, VK_QUEUE_TRANSFER_BIT);
        if (graphicsPool == transferPool)
        {
            array_construct(&set.buffers, &memoryManager->cpu_allocationBindings, 1);
            set.buffers[0] =
            {
                .handle = utils::create_command_buffer(gpu->logicalHandle, graphicsPool->handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY),
                .queueFlags = graphicsPool->queueFlags,
            };
        }
        else
        {
            array_construct(&set.buffers, &memoryManager->cpu_allocationBindings, 2);
            set.buffers[0] =
            {
                .handle = utils::create_command_buffer(gpu->logicalHandle, graphicsPool->handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY),
                .queueFlags = graphicsPool->queueFlags,
            };
            set.buffers[1] =
            {
                .handle = utils::create_command_buffer(gpu->logicalHandle, transferPool->handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY),
                .queueFlags = transferPool->queueFlags,
            };
        }
        return set;
    }

    void vulkan_command_pool_set_destroy(VulkanCommandPoolSet* set, VulkanGpu* gpu, VulkanMemoryManager* memoryManager)
    {
        for (auto it = create_iterator(&set->pools); !is_finished(&it); advance(&it))
        {
            utils::destroy_command_pool(get(it)->handle, gpu->logicalHandle, &memoryManager->cpu_allocationCallbacks);
        }
        array_destruct(&set->pools);
    }

    void vulkan_command_buffer_set_destroy(VulkanCommandBufferSet* set, VulkanGpu* gpu, VulkanMemoryManager* memoryManager)
    {
        array_destruct(&set->buffers);
    }

    VulkanCommandPool* vulkan_get_command_pool(VulkanCommandPoolSet* set, VkQueueFlags queueFlags)
    {
        for (auto it = create_iterator(&set->pools); !is_finished(&it); advance(&it))
        {
            VulkanCommandPool* pool = get(it);
            if (pool->queueFlags & queueFlags)
            {
                return pool;
            }
        }
        return nullptr;
    }

    VulkanCommandBuffer* vulkan_get_command_buffer(VulkanCommandBufferSet* set, VkQueueFlags queueFlags)
    {
        for (auto it = create_iterator(&set->buffers); !is_finished(&it); advance(&it))
        {
            VulkanCommandBuffer* buffer = get(it);
            if (buffer->queueFlags & queueFlags)
            {
                return buffer;
            }
        }
        return nullptr;
    }
}
