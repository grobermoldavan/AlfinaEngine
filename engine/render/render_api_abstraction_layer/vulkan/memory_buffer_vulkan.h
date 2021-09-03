#ifndef AL_MEMORY_BUFFER_VULKAN_H
#define AL_MEMORY_BUFFER_VULKAN_H

#include "../base/memory_buffer.h"
#include "vulkan_base.h"
#include "vulkan_memory_manager.h"

namespace al
{
    struct RenderDeviceVulkan;

    struct MemoryBufferVulkan : MemoryBuffer
    {
        VulkanMemoryManager::Memory memory;
        RenderDeviceVulkan* device;
        VkBuffer handle;
    };

    MemoryBuffer* memory_buffer_vulkan_create(MemoryBufferCreateInfo* createInfo);
    void memory_buffer_vulkan_destroy(MemoryBuffer* buffer);
}

#endif
