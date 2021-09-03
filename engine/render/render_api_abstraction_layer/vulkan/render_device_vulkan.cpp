
#include "render_device_vulkan.h"
#include "vulkan_utils.h"
#include "texture_vulkan.h"

namespace al
{
    // ==================================================================================================================
    //
    //
    // Instance
    //
    //
    // ==================================================================================================================

    VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
                                            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                            void* pUserData) 
    {
        printf("Debug callback : %s\n", pCallbackData->pMessage);
        return VK_FALSE;
    }

    VulkanInstanceCreateResult vulkan_instance_construct(VulkanInstanceCreateInfo* createInfo)
    {
        const bool isDebug = createInfo->isDebug;
        PointerWithSize<const char*> requiredValidationLayers = utils::get_required_validation_layers();
        PointerWithSize<const char*> requiredInstanceExtensions = utils::get_required_instance_extensions();
        if (isDebug)
        {
            Array<VkLayerProperties> availableValidationLayers = utils::get_available_validation_layers(createInfo->frameAllocator);
            defer(array_destruct(&availableValidationLayers));
            for (auto requiredIt = create_iterator(&requiredValidationLayers); !is_finished(&requiredIt); advance(&requiredIt))
            {
                const char* requiredLayerName = *get(requiredIt);
                bool isFound = false;
                for (auto availableIt = create_iterator(&availableValidationLayers); !is_finished(&availableIt); advance(&availableIt))
                {
                    const char* availableLayerName = get(availableIt)->layerName;
                    if (al_vk_strcmp(availableLayerName, requiredLayerName))
                    {
                        isFound = true;
                        break;
                    }
                }
                al_vk_assert_msg(isFound, "Required validation layer is not supported");
            }
        }
        {
            Array<VkExtensionProperties> availableInstanceExtensions = utils::get_available_instance_extensions(createInfo->frameAllocator);
            defer(array_destruct(&availableInstanceExtensions));
            for (auto requiredIt = create_iterator(&requiredInstanceExtensions); !is_finished(&requiredIt); advance(&requiredIt))
            {
                const char* requiredInstanceExtensionName = *get(requiredIt);
                bool isFound = false;
                for (auto availableIt = create_iterator(&availableInstanceExtensions); !is_finished(&availableIt); advance(&availableIt))
                {
                    const char* availableExtensionName = get(availableIt)->extensionName;
                    if (al_vk_strcmp(availableExtensionName, requiredInstanceExtensionName))
                    {
                        isFound = true;
                        break;
                    }
                }
                al_vk_assert_msg(isFound, "Required instance extension is not supported");
            }
        }
        VkApplicationInfo applicationInfo
        {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext              = nullptr,
            .pApplicationName   = createInfo->applicationName,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = EngineConfig::ENGINE_NAME,
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_2
        };
        // @NOTE :  although we setuping debug messenger in the end of this function,
        //          created debug messenger will not be able to recieve information about
        //          vkCreateInstance and vkDestroyInstance calls. So we have to pass additional
        //          VkDebugUtilsMessengerCreateInfoEXT struct pointer to VkInstanceCreateInfo::pNext
        //          to enable debug of these functions
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = utils::get_debug_messenger_create_info(vulkan_debug_callback);
        VkInstanceCreateInfo instanceCreateInfo
        {
            .sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext                      = isDebug ? &debugCreateInfo : nullptr,
            .flags                      = 0,
            .pApplicationInfo           = &applicationInfo,
            .enabledLayerCount          = isDebug ? u32(requiredValidationLayers.size) : 0,
            .ppEnabledLayerNames        = isDebug ? requiredValidationLayers.ptr : nullptr,
            .enabledExtensionCount      = u32(requiredInstanceExtensions.size),
            .ppEnabledExtensionNames    = requiredInstanceExtensions.ptr,
        };
        VkInstance instance;
        VkDebugUtilsMessengerEXT messenger;
        al_vk_check(vkCreateInstance(&instanceCreateInfo, createInfo->callbacks, &instance));
        if (isDebug)
        {
            messenger = utils::create_debug_messenger(&debugCreateInfo, instance, createInfo->callbacks);
        }
        return { instance, messenger };
    }

    void vulkan_instance_destroy(VkInstance instance, VkAllocationCallbacks* callbacks, VkDebugUtilsMessengerEXT messenger)
    {
        if (messenger != VK_NULL_HANDLE)
        {
            utils::destroy_debug_messenger(instance, messenger, callbacks);
        }
        vkDestroyInstance(instance, callbacks);
    }

    // ==================================================================================================================
    //
    //
    // Surface
    //
    //
    // ==================================================================================================================

    VkSurfaceKHR vulkan_surface_create(VulkanSurfaceCreateInfo* createInfo)
    {
#ifdef _WIN32
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo
        {
            .sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext      = nullptr,
            .flags      = 0,
            .hinstance  = ::GetModuleHandle(nullptr),
            .hwnd       = createInfo->window->handle
        };
        VkSurfaceKHR surface;
        al_vk_check(vkCreateWin32SurfaceKHR(createInfo->instance, &surfaceCreateInfo, createInfo->callbacks, &surface));
        return surface;
#else
#   error Unsupported platform
#endif
    }

    void vulkan_surface_destroy(VkSurfaceKHR surface, VkInstance instance, VkAllocationCallbacks* callbacks)
    {
        vkDestroySurfaceKHR(instance, surface, callbacks);
    }

    // ==================================================================================================================
    //
    //
    // Gpu
    //
    //
    // ==================================================================================================================

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
            bool isRequiredExtensionsSupported = utils::does_physical_device_supports_required_extensions(device, utils::get_required_device_extensions(), bindings);
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

    void vulkan_gpu_construct(VulkanGpu* gpu, VulkanGpuCreateInfo* createInfo)
    {
        //
        // Get physical device
        //
        gpu->physicalHandle = pick_physical_device(createInfo->instance, createInfo->surface, createInfo->frameAllocator);
        al_vk_assert(gpu->physicalHandle != VK_NULL_HANDLE);
        //
        // Get command queue infos
        //
        Array<VkQueueFamilyProperties> familyProperties = utils::get_physical_device_queue_family_properties(gpu->physicalHandle, createInfo->frameAllocator);
        defer(array_destruct(&familyProperties));
		Tuple<bool, u32> queuesFamilyIndices[] =
		{
			utils::pick_graphics_queue(familyProperties),
			utils::pick_present_queue(familyProperties, gpu->physicalHandle, createInfo->surface),
			utils::pick_transfer_queue(familyProperties),
		};
        Array<VkDeviceQueueCreateInfo> queueCreateInfos = utils::get_queue_create_infos(queuesFamilyIndices, createInfo->frameAllocator);
        defer(array_destruct(&queueCreateInfos));
        //
        // Create logical device
        //
        VkPhysicalDeviceFeatures deviceFeatures{ };
        fill_required_physical_deivce_features(&deviceFeatures);
        PointerWithSize<const char*> requiredValidationLayers = utils::get_required_validation_layers();
        PointerWithSize<const char*> requiredDeviceExtensions = utils::get_required_device_extensions();
        VkDeviceCreateInfo logicalDeviceCreateInfo
        {
            .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                      = nullptr,
            .flags                      = 0,
            .queueCreateInfoCount       = u32(queueCreateInfos.size),
            .pQueueCreateInfos          = queueCreateInfos.memory,
            .enabledLayerCount          = u32(requiredValidationLayers.size),
            .ppEnabledLayerNames        = requiredValidationLayers.ptr,
            .enabledExtensionCount      = u32(requiredDeviceExtensions.size),
            .ppEnabledExtensionNames    = requiredDeviceExtensions.ptr,
            .pEnabledFeatures           = &deviceFeatures
        };
        al_vk_check(vkCreateDevice(gpu->physicalHandle, &logicalDeviceCreateInfo, createInfo->callbacks, &gpu->logicalHandle));
        //
        // Create queues
        //
        for (auto it = create_iterator(&queueCreateInfos); !is_finished(&it); advance(&it))
        {
            VulkanGpu::CommandQueue* queue = &gpu->commandQueues[to_index(it)];
            queue->queueFamilyIndex = get(it)->queueFamilyIndex;
            if (get<1>(queuesFamilyIndices[0]) == queue->queueFamilyIndex) queue->flags |= VulkanGpu::CommandQueue::GRAPHICS;
            if (get<1>(queuesFamilyIndices[1]) == queue->queueFamilyIndex) queue->flags |= VulkanGpu::CommandQueue::PRESENT;
            if (get<1>(queuesFamilyIndices[2]) == queue->queueFamilyIndex) queue->flags |= VulkanGpu::CommandQueue::TRANSFER;
            vkGetDeviceQueue(gpu->logicalHandle, queue->queueFamilyIndex, 0, &queue->handle);
        }
        //
        // Get device info
        //
        bool hasStencil = utils::pick_depth_stencil_format(gpu->physicalHandle, &gpu->depthStencilFormat);
        if (hasStencil) gpu->flags |= VulkanGpu::HAS_STENCIL;
        vkGetPhysicalDeviceMemoryProperties(gpu->physicalHandle, &gpu->memoryProperties);
        VkPhysicalDeviceVulkan12Properties deviceProperties_12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES, .pNext = nullptr };
        VkPhysicalDeviceVulkan11Properties deviceProperties_11{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES, .pNext = &deviceProperties_12};
        VkPhysicalDeviceProperties2 physcialDeviceProperties2
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &deviceProperties_11,
            .properties = {},
        };
        vkGetPhysicalDeviceProperties2(gpu->physicalHandle, &physcialDeviceProperties2);
        gpu->deviceProperties_10 = physcialDeviceProperties2.properties;
        gpu->deviceProperties_11 = deviceProperties_11;
        gpu->deviceProperties_12 = deviceProperties_12;
    }

    void vulkan_gpu_destroy(VulkanGpu* gpu, VkAllocationCallbacks* callbacks)
    {
        vkDestroyDevice(gpu->logicalHandle, callbacks);
    }

    VkSampleCountFlags vulkan_gpu_get_supported_framebuffer_multisample_types(VulkanGpu* gpu)
    {
        // Is this even correct ?
        return  gpu->deviceProperties_10.limits.framebufferColorSampleCounts &
                gpu->deviceProperties_10.limits.framebufferDepthSampleCounts &
                gpu->deviceProperties_10.limits.framebufferStencilSampleCounts &
                gpu->deviceProperties_10.limits.framebufferNoAttachmentsSampleCounts &
                gpu->deviceProperties_12.framebufferIntegerColorSampleCounts;
    }

    VkSampleCountFlags vulkan_gpu_get_supported_image_multisample_types(VulkanGpu* gpu)
    {
        // Is this even correct ?
        return  gpu->deviceProperties_10.limits.sampledImageColorSampleCounts &
                gpu->deviceProperties_10.limits.sampledImageIntegerSampleCounts &
                gpu->deviceProperties_10.limits.sampledImageDepthSampleCounts &
                gpu->deviceProperties_10.limits.sampledImageStencilSampleCounts &
                gpu->deviceProperties_10.limits.storageImageSampleCounts;
    }

    // ==================================================================================================================
    //
    //
    // Swap Chain
    //
    //
    // ==================================================================================================================

	template<uSize Size>
    void fill_swap_chain_sharing_mode(VkSharingMode* resultMode, u32* resultCount, u32 (&resultFamilyIndices)[Size], VulkanGpu* gpu)
    {
        VulkanGpu::CommandQueue* graphicsQueue = get_command_queue(gpu, VulkanGpu::CommandQueue::GRAPHICS);
        VulkanGpu::CommandQueue* presentQueue = get_command_queue(gpu, VulkanGpu::CommandQueue::PRESENT);
        if (graphicsQueue->queueFamilyIndex == presentQueue->queueFamilyIndex)
        {
            *resultMode = VK_SHARING_MODE_EXCLUSIVE;
            *resultCount = 0;
        }
        else
        {
            *resultMode = VK_SHARING_MODE_CONCURRENT;
            *resultCount = 2;
            resultFamilyIndices[0] = graphicsQueue->queueFamilyIndex;
            resultFamilyIndices[1] = presentQueue->queueFamilyIndex;
        }
    }

    void vulkan_swap_chain_construct(VulkanSwapChain* swapChain, VulkanSwapChainCreateInfo* createInfo)
    {
        {
            utils::SwapChainSupportDetails supportDetails = utils::create_swap_chain_support_details(createInfo->surface, createInfo->gpu->physicalHandle, createInfo->frameAllocator);
            defer(utils::destroy_swap_chain_support_details(&supportDetails));
            VkSurfaceFormatKHR  surfaceFormat   = utils::choose_swap_chain_surface_format(supportDetails.formats);
            VkPresentModeKHR    presentMode     = utils::choose_swap_chain_surface_present_mode(supportDetails.presentModes);
            VkExtent2D          extent          = utils::choose_swap_chain_extent(createInfo->windowWidth, createInfo->windowHeight, &supportDetails.capabilities);
            // @NOTE :  supportDetails.capabilities.maxImageCount == 0 means unlimited amount of images in swapchain
            u32 imageCount = supportDetails.capabilities.minImageCount + 1;
            if (supportDetails.capabilities.maxImageCount != 0 && imageCount > supportDetails.capabilities.maxImageCount)
            {
                imageCount = supportDetails.capabilities.maxImageCount;
            }
            u32 queueFamiliesIndices[VulkanGpu::MAX_UNIQUE_COMMAND_QUEUES] = {};
            VkSwapchainCreateInfoKHR swapChainCreateInfo
            {
                .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext                  = nullptr,
                .flags                  = 0,
                .surface                = createInfo->surface,
                .minImageCount          = imageCount,
                .imageFormat            = surfaceFormat.format,
                .imageColorSpace        = surfaceFormat.colorSpace,
                .imageExtent            = extent,
                .imageArrayLayers       = 1,
                .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount  = 0,
                .pQueueFamilyIndices    = queueFamiliesIndices,
                .preTransform           = supportDetails.capabilities.currentTransform, // Allows to apply transform to images (rotate etc)
                .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode            = presentMode,
                .clipped                = VK_TRUE,
                .oldSwapchain           = VK_NULL_HANDLE,
            };
            fill_swap_chain_sharing_mode(&swapChainCreateInfo.imageSharingMode, &swapChainCreateInfo.queueFamilyIndexCount, queueFamiliesIndices, createInfo->gpu);
            al_vk_check(vkCreateSwapchainKHR(createInfo->gpu->logicalHandle, &swapChainCreateInfo, createInfo->callbacks, &swapChain->handle));
            swapChain->surfaceFormat = surfaceFormat;
            swapChain->extent = extent;
        }
        {
            u32 swapChainImageCount;
            al_vk_check(vkGetSwapchainImagesKHR(createInfo->gpu->logicalHandle, swapChain->handle, &swapChainImageCount, nullptr));
            array_construct(&swapChain->images, createInfo->persistentAllocator, swapChainImageCount);
            Array<VkImage> swapChainImageHandles;
            array_construct(&swapChainImageHandles, createInfo->frameAllocator, swapChainImageCount);
            defer(array_destruct(&swapChainImageHandles));
            // It seems like this vkGetSwapchainImagesKHR call leaks memory
            al_vk_check(vkGetSwapchainImagesKHR(createInfo->gpu->logicalHandle, swapChain->handle, &swapChainImageCount, swapChainImageHandles.memory));
            for (u32 it = 0; it < swapChainImageCount; it++)
            {
                VkImageView view = VK_NULL_HANDLE;
                VkImageViewCreateInfo imageViewCreateInfo
                {
                    .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext              = nullptr,
                    .flags              = 0,
                    .image              = swapChainImageHandles[it],
                    .viewType           = VK_IMAGE_VIEW_TYPE_2D,
                    .format             = swapChain->surfaceFormat.format,
                    .components         = 
                    {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY
                    },
                    .subresourceRange   = 
                    {
                        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                        .baseMipLevel   = 0,
                        .levelCount     = 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1
                    }
                };
                al_vk_check(vkCreateImageView(createInfo->gpu->logicalHandle, &imageViewCreateInfo, createInfo->callbacks, &view));
                swapChain->images[it] =
                {
                    .handle = swapChainImageHandles[it],
                    .view = view,
                };
            }
        }
    }

    void vulkan_swap_chain_destroy(VulkanSwapChain* swapChain, VkDevice device, VkAllocationCallbacks* callbacks)
    {
        for (auto it = create_iterator(&swapChain->images); !is_finished(&it); advance(&it))
        {
            vkDestroyImageView(device, get(it)->view, callbacks);
        }
        array_destruct(&swapChain->images);
        vkDestroySwapchainKHR(device, swapChain->handle, callbacks);
    }

    // ==================================================================================================================
    //
    //
    // Vtable functions
    //
    //
    // ==================================================================================================================

    RenderDevice* vulkan_device_create(RenderDeviceCreateInfo* createInfo)
    {
        RenderDeviceVulkan* device = allocate<RenderDeviceVulkan>(createInfo->persistentAllocator);
        al_vk_memset(device, 0, sizeof(RenderDeviceVulkan));
        VulkanMemoryManagerCreateInfo memoryManagerCreateInfo
        {
            .persistentAllocator    = createInfo->persistentAllocator,
            .frameAllocator         = createInfo->frameAllocator,
        };
        vulkan_memory_manager_construct(&device->memoryManager, &memoryManagerCreateInfo);
        VulkanInstanceCreateInfo instanceCreateInfo
        {
            .persistentAllocator    = createInfo->persistentAllocator,
            .frameAllocator         = createInfo->frameAllocator,
            .callbacks              = &device->memoryManager.cpu_allocationCallbacks,
            .applicationName        = "name",
            .isDebug                = (createInfo->flags & RenderDeviceCreateInfo::IS_DEBUG) != 0,
        };
        auto[instance, debugMessenger] = vulkan_instance_construct(&instanceCreateInfo);
        device->instance = instance;
        device->debugMessenger = debugMessenger;
        VulkanSurfaceCreateInfo surfaceCreateInfo
        {
            .window     = createInfo->window,
            .instance   = device->instance,
            .callbacks  = &device->memoryManager.cpu_allocationCallbacks,
        };
        device->surface = vulkan_surface_create(&surfaceCreateInfo);
        VulkanGpuCreateInfo gpuCreateInfo
        {
            .instance               = device->instance,
            .surface                = device->surface,
            .persistentAllocator    = &device->memoryManager.cpu_persistentAllocator,
            .frameAllocator         = &device->memoryManager.cpu_frameAllocator,
            .callbacks              = &device->memoryManager.cpu_allocationCallbacks,
        };
        vulkan_gpu_construct(&device->gpu, &gpuCreateInfo);
        vulkan_memory_manager_set_device(&device->memoryManager, device->gpu.logicalHandle);
        VulkanSwapChainCreateInfo swapChainCreateInfo
        {
            .gpu                    = &device->gpu,
            .surface                = device->surface,
            .persistentAllocator    = &device->memoryManager.cpu_persistentAllocator,
            .frameAllocator         = &device->memoryManager.cpu_frameAllocator,
            .callbacks              = &device->memoryManager.cpu_allocationCallbacks,
            .windowWidth            = platform_window_get_current_width(createInfo->window),
            .windowHeight           = platform_window_get_current_height(createInfo->window),
        };
        vulkan_swap_chain_construct(&device->swapChain, &swapChainCreateInfo);

        return device;
    }

    void vulkan_device_destroy(RenderDevice* _device)
    {
        RenderDeviceVulkan* device = (RenderDeviceVulkan*)_device;
        vulkan_swap_chain_destroy(&device->swapChain, device->gpu.logicalHandle, &device->memoryManager.cpu_allocationCallbacks);
        vulkan_memory_manager_destroy(&device->memoryManager);
        vulkan_gpu_destroy(&device->gpu, &device->memoryManager.cpu_allocationCallbacks);
        vulkan_surface_destroy(device->surface, device->instance, &device->memoryManager.cpu_allocationCallbacks);
        vulkan_instance_destroy(device->instance, &device->memoryManager.cpu_allocationCallbacks, device->debugMessenger);
    }

    uSize vulkan_get_swap_chain_textures_num(RenderDevice* _device)
    {
        RenderDeviceVulkan* device = (RenderDeviceVulkan*)_device;
        return device->swapChain.images.size;
    }

    Texture* vulkan_get_swap_chain_texture(RenderDevice* _device, uSize index)
    {
        RenderDeviceVulkan* device = (RenderDeviceVulkan*)_device;
        VulkanSwapChain::Image* image = &device->swapChain.images[index];
        TextureVulkan* tex = allocate<TextureVulkan>(&device->memoryManager.cpu_persistentAllocator);
        al_vk_memset(tex, 0, sizeof(TextureVulkan));
        *tex =
        {
            .subresourceRange   =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = VK_REMAINING_MIP_LEVELS,
                .baseArrayLayer = 0,
                .layerCount     = VK_REMAINING_ARRAY_LAYERS,
            },
            .extent             = { device->swapChain.extent.width, device->swapChain.extent.height, 1 },
            .device             = device,
            .image              = image->handle,
            .imageView          = image->view,
            .flags              = TextureVulkan::SWAP_CHAIN_TEXTURE,
            .format             = device->swapChain.surfaceFormat.format,
            .currentLayout      = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        return tex;
    }
}
