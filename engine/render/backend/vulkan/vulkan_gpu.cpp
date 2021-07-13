
#include "vulkan_gpu.h"

namespace al
{
    VulkanGpu::CommandQueue* get_command_queue(VulkanGpu* gpu, VulkanGpu::CommandQueueFlags flags)
    {
        for (auto it = create_iterator(&gpu->commandQueues); !is_finished(&it); advance(&it))
        {
            VulkanGpu::CommandQueue* queue = get(it);
            if (queue->flags & flags)
            {
                return queue;
            }
        }
        return nullptr;
    }

    void fill_required_physical_deivce_features(VkPhysicalDeviceFeatures* features)
    {
        // @TODO : fill features struct
        // features->samplerAnisotropy = true;
    }

    VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface, AllocatorBindings* bindings)
    {
        auto isDeviceSuitable = [](VkPhysicalDevice device, VkSurfaceKHR surface, AllocatorBindings* bindings) -> bool
        {
            //
            // Check if all required queues are supported
            //
            Array<VkQueueFamilyProperties> familyProperties = utils::get_physical_device_queue_family_properties(device, bindings);
            defer(array_destruct(&familyProperties));
            Tuple<bool, u32> graphicsQueue = utils::pick_graphics_queue(familyProperties);
            Tuple<bool, u32> presentQueue = utils::pick_present_queue(familyProperties, device, surface);
            Tuple<bool, u32> transferQueue = utils::pick_transfer_queue(familyProperties);
            bool isRequiredExtensionsSupported = utils::does_physical_device_supports_required_extensions(device, { .ptr = utils::DEVICE_EXTENSIONS, .size = array_size(utils::DEVICE_EXTENSIONS) }, bindings);
            bool isSwapChainSuppoted = false;
            if (isRequiredExtensionsSupported)
            {
                utils::SwapChainSupportDetails supportDetails = utils::create_swap_chain_support_details(surface, device, bindings);
                isSwapChainSuppoted = supportDetails.formats.size && supportDetails.presentModes.size;
                utils::destroy_swap_chain_support_details(&supportDetails);
            }
            VkPhysicalDeviceFeatures requiredFeatures{};
            fill_required_physical_deivce_features(&requiredFeatures);
            bool isRequiredFeaturesSupported = utils::does_physical_device_supports_required_features(device, &requiredFeatures);
            return
            {
                get<0>(graphicsQueue) && get<0>(presentQueue) && get<0>(transferQueue) &&
                isRequiredExtensionsSupported && isSwapChainSuppoted && isRequiredFeaturesSupported
            };
        };
        Array<VkPhysicalDevice> available = utils::get_available_physical_devices(instance, bindings);
        defer(array_destruct(&available));
        for (auto it = create_iterator(&available); !is_finished(&it); advance(&it))
        {
            if (isDeviceSuitable(*get(it), surface, bindings))
            {
                // @TODO :  log device name and shit
                // VkPhysicalDeviceProperties deviceProperties;
                // vkGetPhysicalDeviceProperties(chosenPhysicalDevice, &deviceProperties);
                return *get(it);
            }
        }
        return VK_NULL_HANDLE;
    }

    void construct_gpu(VulkanGpu* gpu, VkInstance instance, VkSurfaceKHR surface, AllocatorBindings* bindings, VkAllocationCallbacks* callbacks)
    {
        //
        // Get physical device
        //
        gpu->physicalHandle = pick_physical_device(instance, surface, bindings);
        al_vk_assert(gpu->physicalHandle != VK_NULL_HANDLE);
        //
        // Get command queue infos
        //
        Array<VkQueueFamilyProperties> familyProperties = utils::get_physical_device_queue_family_properties(gpu->physicalHandle, bindings);
        defer(array_destruct(&familyProperties));
		StaticPointerWithSize<Tuple<bool, u32>, 3> queuesFamilyIndices;
		queuesFamilyIndices[0] = utils::pick_graphics_queue(familyProperties);
		queuesFamilyIndices[1] = utils::pick_present_queue(familyProperties, gpu->physicalHandle, surface);
		queuesFamilyIndices[2] = utils::pick_transfer_queue(familyProperties);

        Array<VkDeviceQueueCreateInfo> queueCreateInfos = utils::get_queue_create_infos(queuesFamilyIndices, bindings);
        defer(array_destruct(&queueCreateInfos));
        //
        // Create logical device
        //
        VkPhysicalDeviceFeatures deviceFeatures{ };
        fill_required_physical_deivce_features(&deviceFeatures);
        VkDeviceCreateInfo logicalDeviceCreateInfo
        {
            .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                      = nullptr,
            .flags                      = 0,
            .queueCreateInfoCount       = static_cast<u32>(queueCreateInfos.size),
            .pQueueCreateInfos          = queueCreateInfos.memory,
            .enabledLayerCount          = array_size(utils::VALIDATION_LAYERS),
            .ppEnabledLayerNames        = utils::VALIDATION_LAYERS,
            .enabledExtensionCount      = array_size(utils::DEVICE_EXTENSIONS),
            .ppEnabledExtensionNames    = utils::DEVICE_EXTENSIONS,
            .pEnabledFeatures           = &deviceFeatures
        };
        al_vk_check(vkCreateDevice(gpu->physicalHandle, &logicalDeviceCreateInfo, callbacks, &gpu->logicalHandle));
        //
        // Create queues and get other stuff
        //
        array_construct(&gpu->commandQueues, bindings, queueCreateInfos.size);
        for (auto it = create_iterator(&gpu->commandQueues); !is_finished(&it); advance(&it))
        {
            VulkanGpu::CommandQueue* queue = get(it);
            queue->queueFamilyIndex = queueCreateInfos[to_index(it)].queueFamilyIndex;
            if (get<1>(queuesFamilyIndices[0]) == queue->queueFamilyIndex) queue->flags |= VulkanGpu::CommandQueue::GRAPHICS;
            if (get<1>(queuesFamilyIndices[1]) == queue->queueFamilyIndex) queue->flags |= VulkanGpu::CommandQueue::PRESENT;
            if (get<1>(queuesFamilyIndices[2]) == queue->queueFamilyIndex) queue->flags |= VulkanGpu::CommandQueue::TRANSFER;
            vkGetDeviceQueue(gpu->logicalHandle, queue->queueFamilyIndex, 0, &queue->handle);
        }
        bool hasStencil = utils::pick_depth_stencil_format(gpu->physicalHandle, &gpu->depthStencilFormat);
        if (hasStencil) gpu->flags |= VulkanGpu::HAS_STENCIL;
        vkGetPhysicalDeviceMemoryProperties(gpu->physicalHandle, &gpu->memoryProperties);
    }

    void destroy_gpu(VulkanGpu* gpu, VkAllocationCallbacks* callbacks)
    {
        vkDestroyDevice(gpu->logicalHandle, callbacks);
    }
}
