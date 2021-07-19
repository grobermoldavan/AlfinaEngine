
#include "vulkan_utils_initializers.h"

namespace al::utils::initializers
{
    VkSemaphoreCreateInfo semaphore_create_info()
    {
        return
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
    }

    VkFenceCreateInfo fence_create_info()
    {
        return
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
    }

    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags)
    {
        return
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext              = nullptr,
            .flags              = flags,
            .pInheritanceInfo   = nullptr,
        };
    }

    VkSubmitInfo submit_info(VkCommandBuffer* buffers, u32 buffersCount)
    {
        return
        {
            .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                  = nullptr,
            .waitSemaphoreCount     = 0,
            .pWaitSemaphores        = nullptr,
            .pWaitDstStageMask      = 0,
            .commandBufferCount     = buffersCount,
            .pCommandBuffers        = buffers,
            .signalSemaphoreCount   = 0,
            .pSignalSemaphores      = nullptr,
        };
    }
}
