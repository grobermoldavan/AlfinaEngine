
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

    VkCommandBufferBeginInfo command_buffer_begin_info()
    {
        return
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext              = nullptr,
            .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo   = nullptr,
        };
    }
}
