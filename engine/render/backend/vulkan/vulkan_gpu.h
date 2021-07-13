#ifndef AL_VULKAN_GPU_H
#define AL_VULKAN_GPU_H

#include "engine/types.h"
#include "engine/utilities/utilities.h"
#include "engine/memory/memory.h"
#include "vulkan_base.h"

namespace al
{
    struct VulkanGpu
    {
        static constexpr uSize MAX_UNIQUE_COMMAND_QUEUES = 3;
        using CommandQueueFlags = u32;
        using GpuFlags = u64;
        enum Flags : GpuFlags
        {
            HAS_STENCIL = GpuFlags(1) << 0,
        };
        struct CommandQueue
        {
            enum Flags : CommandQueueFlags
            {
                GRAPHICS    = CommandQueueFlags(1) << 0,
                PRESENT     = CommandQueueFlags(1) << 1,
                TRANSFER    = CommandQueueFlags(1) << 2,
            };
            VkQueue handle;
            u32 queueFamilyIndex;
            CommandQueueFlags flags;
        };
        Array<CommandQueue> commandQueues;
        VkPhysicalDevice physicalHandle;
        VkDevice logicalHandle;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        VkFormat depthStencilFormat;
        GpuFlags flags;
        // available memory and stuff...
    };

    VulkanGpu::CommandQueue* get_command_queue(VulkanGpu* gpu, VulkanGpu::CommandQueueFlags flags);
    void fill_required_physical_deivce_features(VkPhysicalDeviceFeatures* features);
    VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface, AllocatorBindings* bindings);
    void construct_gpu(VulkanGpu* gpu, VkInstance instance, VkSurfaceKHR surface, AllocatorBindings* bindings, VkAllocationCallbacks* callbacks);
    void destroy_gpu(VulkanGpu* gpu, VkAllocationCallbacks* callbacks);
}

#endif
