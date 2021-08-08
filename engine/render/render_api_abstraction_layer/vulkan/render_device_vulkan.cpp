
#include "render_device_vulkan.h"
#include "vulkan_utils.h"
#include "texture_vulkan.h"

namespace al
{
    void vulkan_memory_manager_construct(VulkanMemoryManager* memoryManager, VulkanMemoryManagerCreateInfo* createInfo)
    {
        memoryManager->cpu_persistentAllocator = *createInfo->persistentAllocator;
        memoryManager->cpu_frameAllocator = *createInfo->frameAllocator;
        memoryManager->cpu_allocationCallbacks =
        {
            .pUserData = memoryManager,
            .pfnAllocation = 
                [](void* pUserData, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    VulkanMemoryManager* manager = static_cast<VulkanMemoryManager*>(pUserData);
                    void* result = allocate(&manager->cpu_persistentAllocator, size);
                    al_vk_assert(manager->cpu_currentNumberOfAllocations < VulkanMemoryManager::MAX_CPU_ALLOCATIONS);
                    manager->cpu_allocations[manager->cpu_currentNumberOfAllocations++] =
                    {
                        .ptr = result,
                        .size = size,
                    };
                    return result;
                },
            .pfnReallocation =
                [](void* pUserData, void* pOriginal, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    VulkanMemoryManager* manager = static_cast<VulkanMemoryManager*>(pUserData);
                    void* result = nullptr;
                    for (uSize it = 0; it < manager->cpu_currentNumberOfAllocations; it++)
                    {
                        if (manager->cpu_allocations[it].ptr == pOriginal)
                        {
                            deallocate(&manager->cpu_persistentAllocator, manager->cpu_allocations[it].ptr, manager->cpu_allocations[it].size);
                            result = allocate(&manager->cpu_persistentAllocator, size);
                            manager->cpu_allocations[it] =
                            {
                                .ptr = result,
                                .size = size,
                            };
                            break;
                        }
                    }
                    return result;
                },
            .pfnFree =
                [](void* pUserData, void* pMemory)
                {
                    VulkanMemoryManager* manager = static_cast<VulkanMemoryManager*>(pUserData);
                    for (uSize it = 0; it < manager->cpu_currentNumberOfAllocations; it++)
                    {
                        if (manager->cpu_allocations[it].ptr == pMemory)
                        {
                            deallocate(&manager->cpu_persistentAllocator, manager->cpu_allocations[it].ptr, manager->cpu_allocations[it].size);
                            manager->cpu_allocations[it] = manager->cpu_allocations[manager->cpu_currentNumberOfAllocations - 1];
                            manager->cpu_currentNumberOfAllocations -= 1;
                            break;
                        }
                    }
                },
            .pfnInternalAllocation  = nullptr,
            .pfnInternalFree        = nullptr
        };
        array_construct(&memoryManager->gpu_chunks, &memoryManager->cpu_persistentAllocator, VulkanMemoryManager::GPU_MAX_CHUNKS);
        array_construct(&memoryManager->gpu_ledgers, &memoryManager->cpu_persistentAllocator, VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES * VulkanMemoryManager::GPU_MAX_CHUNKS);
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            chunk->ledger = &memoryManager->gpu_ledgers[it * VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES];
        }
    }

    void vulkan_memory_manager_destroy(VulkanMemoryManager* memoryManager)
    {
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (chunk->memory)
            {
                vkFreeMemory(memoryManager->device, chunk->memory, &memoryManager->cpu_allocationCallbacks);
            }
        }
        array_destruct(&memoryManager->gpu_chunks);
        array_destruct(&memoryManager->gpu_ledgers);
    }

    void vulkan_memory_manager_set_device(VulkanMemoryManager* memoryManager, VkDevice device)
    {
        memoryManager->device = device;
    }

    bool gpu_is_valid_memory(VulkanMemoryManager::Memory memory)
    {
        return memory.memory != VK_NULL_HANDLE;
    }

    VulkanMemoryManager::Memory gpu_allocate(VulkanMemoryManager* memoryManager, VulkanMemoryManager::GpuAllocationRequest request)
    {
        auto setInUse = [](VulkanMemoryManager::GpuMemoryChunk* chunk, uSize numBlocks, uSize startBlock)
        {
            for (uSize it = 0; it < numBlocks; it++)
            {
                uSize currentByte = (startBlock + it) / 8;
                uSize currentBit = (startBlock + it) % 8;
                u8* byte = &chunk->ledger[currentByte];
                *byte |= (1 << currentBit);
            }
            chunk->usedMemoryBytes += numBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            al_vk_assert(chunk->usedMemoryBytes <= VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        };
        al_vk_assert(request.sizeBytes <= VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        uSize requiredNumberOfBlocks = 1 + ((request.sizeBytes - 1) / VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES);
        // 1. Try to find memory in available chunks
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (!chunk->memory)
            {
                continue;
            }
            if (chunk->memoryTypeIndex != request.memoryTypeIndex)
            {
                continue;
            }
            if ((VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES - chunk->usedMemoryBytes) < request.sizeBytes)
            {
                continue;
            }
            uSize inChunkOffset = memory_chunk_find_aligned_free_space(chunk, requiredNumberOfBlocks, request.alignment);
            if (inChunkOffset == VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES)
            {
                continue;
            }
            setInUse(chunk, requiredNumberOfBlocks, inChunkOffset);
            return
            {
                .memory = chunk->memory,
                .offsetBytes = inChunkOffset * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
                .sizeBytes = requiredNumberOfBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
            };
        }
        // 2. Try to allocate new chunk
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (chunk->memory)
            {
                continue;
            }
            // Allocating new chunk
            {
                VkMemoryAllocateInfo memoryAllocateInfo = { };
                memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                memoryAllocateInfo.allocationSize = VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES;
                memoryAllocateInfo.memoryTypeIndex = request.memoryTypeIndex;
                al_vk_check(vkAllocateMemory(memoryManager->device, &memoryAllocateInfo, &memoryManager->cpu_allocationCallbacks, &chunk->memory));
                chunk->memoryTypeIndex = request.memoryTypeIndex;
                std::memset(chunk->ledger, 0, VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES);
            }
            // Allocating memory in new chunk
            uSize inChunkOffset = memory_chunk_find_aligned_free_space(chunk, requiredNumberOfBlocks, request.alignment);
            al_vk_assert(inChunkOffset != VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES);
            setInUse(chunk, requiredNumberOfBlocks, inChunkOffset);
            return
            {
                .memory = chunk->memory,
                .offsetBytes = inChunkOffset * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
                .sizeBytes = requiredNumberOfBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
            };
        }
        al_vk_assert_fail("Out of memory");
        return { };
    }

    void gpu_deallocate(VulkanMemoryManager* memoryManager, VulkanMemoryManager::Memory allocation)
    {
        auto setFree = [](VulkanMemoryManager::GpuMemoryChunk* chunk, uSize numBlocks, uSize startBlock)
        {
            for (uSize it = 0; it < numBlocks; it++)
            {
                uSize currentByte = (startBlock + it) / 8;
                uSize currentBit = (startBlock + it) % 8;
                u8* byte = &chunk->ledger[currentByte];
                *byte &= ~(1 << currentBit);
            }
            chunk->usedMemoryBytes -= numBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            al_vk_assert(chunk->usedMemoryBytes <= VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        };
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (chunk->memory != allocation.memory)
            {
                continue;
            }
            uSize startBlock = allocation.offsetBytes / VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            uSize numBlocks = allocation.sizeBytes / VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            setFree(chunk, numBlocks, startBlock);
            if (chunk->usedMemoryBytes == 0)
            {
                // Chunk is empty, so we free it
                vkFreeMemory(memoryManager->device, chunk->memory, &memoryManager->cpu_allocationCallbacks);
                chunk->memory = VK_NULL_HANDLE;
            }
            break;
        }
    }

    uSize memory_chunk_find_aligned_free_space(VulkanMemoryManager::GpuMemoryChunk* chunk, uSize requiredNumberOfBlocks, uSize alignment)
    {
        uSize freeCount = 0;
        uSize startBlock = 0;
        uSize currentBlock = 0;
        for (uSize ledgerIt = 0; ledgerIt < VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES; ledgerIt++)
        {
            u8 byte = chunk->ledger[ledgerIt];
            if (byte == 255)
            {
                freeCount = 0;
                currentBlock += 8;
                continue;
            }
            for (uSize byteIt = 0; byteIt < 8; byteIt++)
            {
                if (byte & (1 << byteIt))
                {
                    freeCount = 0;
                }
                else
                {
                    if (freeCount == 0)
                    {
                        uSize currentBlockOffsetBytes = (ledgerIt * 8 + byteIt) * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
                        bool isAlignedCorrectly = (currentBlockOffsetBytes % alignment) == 0;
                        if (isAlignedCorrectly)
                        {
                            startBlock = currentBlock;
                            freeCount += 1;
                        }
                    }
                    else
                    {
                        freeCount += 1;
                    }
                }
                currentBlock += 1;
                if (freeCount == requiredNumberOfBlocks)
                {
                    goto block_found;
                }
            }
        }
        startBlock = VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES;
        block_found:
        return startBlock;
    }

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
            .apiVersion         = VK_API_VERSION_1_0
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
		StaticPointerWithSize<Tuple<bool, u32>, 3> queuesFamilyIndices;
		queuesFamilyIndices[0] = utils::pick_graphics_queue(familyProperties);
		queuesFamilyIndices[1] = utils::pick_present_queue(familyProperties, gpu->physicalHandle, createInfo->surface);
		queuesFamilyIndices[2] = utils::pick_transfer_queue(familyProperties);
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
        // Create queues and get other stuff
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
        bool hasStencil = utils::pick_depth_stencil_format(gpu->physicalHandle, &gpu->depthStencilFormat);
        if (hasStencil) gpu->flags |= VulkanGpu::HAS_STENCIL;
        vkGetPhysicalDeviceMemoryProperties(gpu->physicalHandle, &gpu->memoryProperties);
    }

    void vulkan_gpu_destroy(VulkanGpu* gpu, VkAllocationCallbacks* callbacks)
    {
        vkDestroyDevice(gpu->logicalHandle, callbacks);
    }

    // ==================================================================================================================
    //
    //
    // Swap Chain
    //
    //
    // ==================================================================================================================

    void fill_swap_chain_sharing_mode(VkSharingMode* resultMode, u32* resultCount, PointerWithSize<u32> resultFamilyIndices, VulkanGpu* gpu)
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
            StaticPointerWithSize<u32, VulkanGpu::MAX_UNIQUE_COMMAND_QUEUES> queueFamiliesIndices = {};
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
                .pQueueFamilyIndices    = queueFamiliesIndices.ptr,
                .preTransform           = supportDetails.capabilities.currentTransform, // Allows to apply transform to images (rotate etc)
                .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode            = presentMode,
                .clipped                = VK_TRUE,
                .oldSwapchain           = VK_NULL_HANDLE,
            };
            fill_swap_chain_sharing_mode(&swapChainCreateInfo.imageSharingMode, &swapChainCreateInfo.queueFamilyIndexCount, { .ptr = queueFamiliesIndices.ptr, .size = queueFamiliesIndices.size }, createInfo->gpu);
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
