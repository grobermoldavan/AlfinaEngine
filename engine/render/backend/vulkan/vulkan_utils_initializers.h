#ifndef AL_UTILS_INITIALIZERS_H
#define AL_UTILS_INITIALIZERS_H

#include "vulkan_base.h"

namespace al::utils::initializers
{
    VkSemaphoreCreateInfo semaphore_create_info();
    VkFenceCreateInfo fence_create_info();
    VkCommandBufferBeginInfo command_buffer_begin_info();
}

#endif
