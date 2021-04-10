
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "engine/config.h"
#include "engine/render/renderer_backend_vulkan.h"

#define log_msg(format, ...) std::fprintf(stdout, format, __VA_ARGS__);

#define al_vk_check(cmd) do { VkResult result = cmd; assert(result == VK_SUCCESS); } while(0)

namespace al
{
    template<typename T>
    uSize vulkan_array_view_memory_size(RendererBackendVulkanArrayView<T> view)
    {
        return view.count * sizeof(T);
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Allocation
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_set_allocation_callbacks(RendererBackendVulkan* backend)
    {
        // @TODO :  support reallocation in engine/memory allocators, so they can be used as allocation callbacks for vulkan
        // log_msg("1. Setting allocation callbacks\n");
        backend->vkAllocationCallbacks =
        {
            .pUserData = nullptr,
            .pfnAllocation = 
                [](void* pUserData, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    // log_msg("Allocating %d bytes of memory with alignment of %d bytes. Allocation scope is %s\n", (int)size, (int)alignment, RendererBackendVulkan::ALLOCATION_SCOPE_TO_STR[allocationScope]);
                    void* mem = std::malloc(size); std::memset(mem, 0, size); return mem;
                },
            .pfnReallocation =
                [](void* pUserData, void* pOriginal, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    // log_msg("Reallocating %d bytes of memory with alignment of %d bytes. Allocation scope is %s\n", (int)size, (int)alignment, RendererBackendVulkan::ALLOCATION_SCOPE_TO_STR[allocationScope]);
                    return std::realloc(pOriginal, size);
                },
            .pfnFree =
                [](void* pUserData, void* pMemory)
                {
                    // log_msg("Freeing memory from %p\n", pMemory);
                    std::free(pMemory);
                },
            .pfnInternalAllocation  = nullptr,
            .pfnInternalFree        = nullptr
        };
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Debug
    // =================================================================================================================================================
    // =================================================================================================================================================

    VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
                                            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                            void* pUserData) 
    {
        log_msg("Debug callback : %s\n", pCallbackData->pMessage);;
        return VK_FALSE;
    }

    VkDebugUtilsMessengerCreateInfoEXT vulkan_get_debug_messenger_create_info()
    {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{ };
        createInfo.sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity  = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback  = vulkan_debug_callback;
        createInfo.pUserData        = nullptr;
        return createInfo;
    }

    void vulkan_setup_debug_messenger(RendererBackendVulkan* backend)
    {
        // @NOTE :  adresses of extension functions must be found manually via vkGetInstanceProcAddr
        auto CreateDebugUtilsMessengerEXT = [](VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) -> VkResult
        {
            PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func)
            {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            }
            else
            {
                return VK_ERROR_EXTENSION_NOT_PRESENT;
            }
        };
        VkDebugUtilsMessengerCreateInfoEXT createInfo = vulkan_get_debug_messenger_create_info();
        CreateDebugUtilsMessengerEXT(backend->vkInstance, &createInfo, &backend->vkAllocationCallbacks, &backend->vkDebugMessenger);
    }

    void vulkan_destroy_debug_messenger(RendererBackendVulkan* backend)
    {
        // @NOTE :  adresses of extension functions must be found manually via vkGetInstanceProcAddr
        auto DestroyDebugUtilsMessengerEXT = [](VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
        {
            PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func)
            {
                func(instance, messenger, pAllocator);
            }
        };
        DestroyDebugUtilsMessengerEXT(backend->vkInstance, backend->vkDebugMessenger, &backend->vkAllocationCallbacks);
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // VkInstance creation
    // =================================================================================================================================================
    // =================================================================================================================================================

    RendererBackendVulkanArrayView<VkLayerProperties> vulkan_get_available_validation_layers(AllocatorBindings* bindings)
    {
        u32 count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        VkLayerProperties* availablevalidationLayers = static_cast<VkLayerProperties*>(allocate(bindings, count * sizeof(VkLayerProperties)));
        std::memset(availablevalidationLayers, 0, count * sizeof(VkLayerProperties));
        vkEnumerateInstanceLayerProperties(&count, availablevalidationLayers);
        return
        {
            .memory = availablevalidationLayers,
            .count = count
        };
    }

    RendererBackendVulkanArrayView<VkExtensionProperties> vulkan_get_available_extensions(AllocatorBindings* bindings)
    {
        u32 count;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        VkExtensionProperties* availableExtensions = static_cast<VkExtensionProperties*>(allocate(bindings, count * sizeof(VkExtensionProperties)));
        std::memset(availableExtensions, 0, count * sizeof(VkExtensionProperties));
        vkEnumerateInstanceExtensionProperties(nullptr, &count, availableExtensions);
        return
        {
            .memory = availableExtensions,
            .count = count
        };
    }

    void vulkan_create_vk_instance(RendererBackendVulkan* backend, RendererBackendInitData* initData)
    {
        if (initData->isDebug)
        {
            RendererBackendVulkanArrayView<VkLayerProperties> availablevalidationLayers = vulkan_get_available_validation_layers(&backend->bindings);
            for (uSize requiredIt = 0; requiredIt < RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS_COUNT; requiredIt++)
            {
                const char* requiredLayerName = RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS[requiredIt];
                bool isFound = false;
                for (uSize availableIt = 0; availableIt < availablevalidationLayers.count; availableIt++)
                {
                    const char* availableLayerName = availablevalidationLayers.memory[availableIt].layerName;
                    // @TODO :  replace default strcmp
                    if (std::strcmp(availableLayerName, requiredLayerName) == 0)
                    {
                        isFound = true;
                        break;
                    }
                }
                // @TODO :  replace default assert
                assert(isFound);
            }
            deallocate(&backend->bindings, availablevalidationLayers.memory, vulkan_array_view_memory_size(availablevalidationLayers));
        }
        {
            RendererBackendVulkanArrayView<VkExtensionProperties> availableExtensions = vulkan_get_available_extensions(&backend->bindings);
            for (uSize requiredIt = 0; requiredIt < RendererBackendVulkan::REQUIRED_EXTENSIONS_COUNT; requiredIt++)
            {
                const char* requiredExtensionName = RendererBackendVulkan::REQUIRED_EXTENSIONS[requiredIt];
                bool isFound = false;
                for (uSize availableIt = 0; availableIt < availableExtensions.count; availableIt++)
                {
                    const char* availableExtensionName = availableExtensions.memory[availableIt].extensionName;
                    // @TODO :  replace default strcmp
                    if (std::strcmp(availableExtensionName, requiredExtensionName) == 0)
                    {
                        isFound = true;
                        break;
                    }
                }
                // @TODO :  replace default assert
                assert(isFound);
            }
            deallocate(&backend->bindings, availableExtensions.memory, vulkan_array_view_memory_size(availableExtensions));
        }
        // log_msg("3. Creating VkApplicationInfo struct (this step is optional)\n");
        VkApplicationInfo applicationInfo
        {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext              = nullptr,
            .pApplicationName   = initData->applicationName,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = EngineConfig::ENGINE_NAME,
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_0
        };
        // @NOTE :  although we setuping debug messenger via __setup_debug_messenger call,
        //          created debug messenger will not be able to recieve information about
        //          vkCreateInstance and vkDestroyInstance calls. So we have to pass additional
        //          VkDebugUtilsMessengerCreateInfoEXT struct pointer to VkInstanceCreateInfo::pNext
        //          to enable debug of these functions
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vulkan_get_debug_messenger_create_info();
        VkInstanceCreateInfo instanceCreateInfo
        {
            .sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext                      = &debugCreateInfo,
            .flags                      = { },
            .pApplicationInfo           = &applicationInfo,
            .enabledLayerCount          = RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS_COUNT,
            .ppEnabledLayerNames        = RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS,
            .enabledExtensionCount      = RendererBackendVulkan::REQUIRED_EXTENSIONS_COUNT,
            .ppEnabledExtensionNames    = RendererBackendVulkan::REQUIRED_EXTENSIONS,
        };
        al_vk_check(vkCreateInstance(&instanceCreateInfo, &backend->vkAllocationCallbacks, &backend->vkInstance));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Surface
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_surface(RendererBackendVulkan* backend)
    {
        // @TODO :  move surface creation to platform layer
        // log_msg("8. Creating surface (will be used to display result image to screen)\n");
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo
        {
            .sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext      = nullptr,
            .flags      = { },
            .hinstance  = ::GetModuleHandle(nullptr),
            .hwnd       = backend->window->handle
        };
        al_vk_check(vkCreateWin32SurfaceKHR(backend->vkInstance, &surfaceCreateInfo, &backend->vkAllocationCallbacks, &backend->vkSurface));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Physical device queue families info
    // =================================================================================================================================================
    // =================================================================================================================================================

    RendererBackendVulkanArrayView<VkQueueFamilyProperties> vulkan_get_queue_family_properties(RendererBackendVulkan* backend, VkPhysicalDevice device)
    {
        u32 count;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        VkQueueFamilyProperties* available = static_cast<VkQueueFamilyProperties*>(allocate(&backend->bindings, count * sizeof(VkQueueFamilyProperties)));
        std::memset(available, 0, count * sizeof(VkQueueFamilyProperties));
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, available);
        return
        {
            .memory = available,
            .count = count
        };
    }

    bool vulkan_is_queue_families_info_complete(VulkanQueueFamiliesInfo* info)
    {
        return  info->familyInfoIndex[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].isPresent &&
                info->familyInfoIndex[VulkanQueueFamiliesInfo::SURFACE_PRESENT_FAMILY].isPresent;
    }

    VulkanQueueFamiliesInfo vulkan_get_queue_families_info(RendererBackendVulkan* backend, VkPhysicalDevice device)
    {
        VkBool32 isSupported;
        VulkanQueueFamiliesInfo info{ };
        RendererBackendVulkanArrayView<VkQueueFamilyProperties> available = vulkan_get_queue_family_properties(backend, device);
        for (uSize it = 0; it < available.count; it++)
        {
            if (available.memory[it].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                info.familyInfoIndex[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].index = it;
                info.familyInfoIndex[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].isPresent = true;
            }
            if (isSupported = false, vkGetPhysicalDeviceSurfaceSupportKHR(device, it, backend->vkSurface, &isSupported), isSupported)
            {
                info.familyInfoIndex[VulkanQueueFamiliesInfo::SURFACE_PRESENT_FAMILY].index = it;
                info.familyInfoIndex[VulkanQueueFamiliesInfo::SURFACE_PRESENT_FAMILY].isPresent = true;
            }
            if (vulkan_is_queue_families_info_complete(&info))
            {
                break;
            }
        }
        deallocate(&backend->bindings, available.memory, vulkan_array_view_memory_size(available));
        return info;
    }

    RendererBackendVulkanArrayView<VkDeviceQueueCreateInfo> vulkan_get_queue_create_infos(RendererBackendVulkan* backend, VulkanQueueFamiliesInfo* queueFamiliesInfo)
    {
        // @NOTE :  this is possible that queue family might support more than one of the required features,
        //          so we have to remove duplicates from queueFamiliesInfo and create VkDeviceQueueCreateInfos
        //          only for unique indexes
        const f32 QUEUE_DEFAULT_PRIORITY = 1.0f;
        bool isFound = false;
        u32 indicesArray[VulkanQueueFamiliesInfo::FAMILIES_NUM];
        RendererBackendVulkanArrayView<u32> uniqueQueueIndices
        {
            .memory = indicesArray,
            .count = 0
        };
        for (uSize it = 0; it < VulkanQueueFamiliesInfo::FAMILIES_NUM; it++)
        {
            isFound = false;
            for (uSize uniqueIt = 0; uniqueIt < uniqueQueueIndices.count; uniqueIt++)
            {
                if (uniqueQueueIndices.memory[uniqueIt] == queueFamiliesInfo->familyInfoIndex[it].index)
                {
                    isFound = true;
                    break;
                }
            }
            if (isFound)
            {
                continue;
            }
            uniqueQueueIndices.count += 1;
            uniqueQueueIndices.memory[uniqueQueueIndices.count - 1] = queueFamiliesInfo->familyInfoIndex[it].index;
        }
        RendererBackendVulkanArrayView<VkDeviceQueueCreateInfo> result
        {
            .memory = static_cast<VkDeviceQueueCreateInfo*>(allocate(&backend->bindings, uniqueQueueIndices.count * sizeof(VkDeviceQueueCreateInfo))),
            .count = uniqueQueueIndices.count
        };
        for (uSize it = 0; it < result.count; it++)
        {
            result.memory[it] = 
            {
                .sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = { },
                .queueFamilyIndex   = uniqueQueueIndices.memory[it],
                .queueCount         = 1,
                .pQueuePriorities   = &QUEUE_DEFAULT_PRIORITY,
            };
        }
        return result;
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Physical device
    // =================================================================================================================================================
    // =================================================================================================================================================

    bool vulkan_does_physical_device_supports_required_extensions(RendererBackendVulkan* backend, VkPhysicalDevice device)
    {
        // log_msg("Checking required device extensions (not the same as extensions that was before).\n");
        u32 count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
        RendererBackendVulkanArrayView<VkExtensionProperties> availableExtensions
        {
            .memory = static_cast<VkExtensionProperties*>(allocate(&backend->bindings, count * sizeof(VkExtensionProperties))),
            .count = count
        };
        vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensions.count, availableExtensions.memory);
        bool isRequiredExtensionAvailable;
        bool result = true;
        for (uSize requiredIt = 0; requiredIt < RendererBackendVulkan::REQUIRED_DEVICE_EXTENSIONS_COUNT; requiredIt++)
        {
            isRequiredExtensionAvailable = false;
            for (uSize availableIt = 0; availableIt < availableExtensions.count; availableIt++)
            {
                if (std::strcmp(availableExtensions.memory[availableIt].extensionName, RendererBackendVulkan::REQUIRED_DEVICE_EXTENSIONS[requiredIt]) == 0)
                {
                    isRequiredExtensionAvailable = true;
                    break;
                }
            }
            if (!isRequiredExtensionAvailable)
            {
                result = false;
                break;
            }
        }
        deallocate(&backend->bindings, availableExtensions.memory, vulkan_array_view_memory_size(availableExtensions));
        return result;
    }

    bool vulkan_is_physical_device_suitable(RendererBackendVulkan* backend, VkPhysicalDevice device)
    {
        VulkanQueueFamiliesInfo queueFamiliesInfo = vulkan_get_queue_families_info(backend, device);
        bool isQueueFamiliesInfoComplete = vulkan_is_queue_families_info_complete(&queueFamiliesInfo);
        bool isRequiredExtensionsSupported = vulkan_does_physical_device_supports_required_extensions(backend, device);
        bool isSwapChainSuppoted = true;
        if (isRequiredExtensionsSupported)
        {
            RendererBackendVulkanSwapChainSupportDetails supportDetails = vulkan_get_swap_chain_support_details(backend, device);
            isSwapChainSuppoted = supportDetails.formats.count && supportDetails.presentModes.count;
            deallocate(&backend->bindings, supportDetails.formats.memory, vulkan_array_view_memory_size(supportDetails.formats));
            deallocate(&backend->bindings, supportDetails.presentModes.memory, vulkan_array_view_memory_size(supportDetails.presentModes));
        }
        return isQueueFamiliesInfoComplete && isRequiredExtensionsSupported && isSwapChainSuppoted;
    }

    RendererBackendVulkanArrayView<VkPhysicalDevice> vulkan_get_available_physical_devices(RendererBackendVulkan* backend)
    {
        u32 count;
        vkEnumeratePhysicalDevices(backend->vkInstance, &count, nullptr);
        VkPhysicalDevice* available = static_cast<VkPhysicalDevice*>(allocate(&backend->bindings, count * sizeof(VkPhysicalDevice)));
        std::memset(available, 0, count * sizeof(VkPhysicalDevice));
        vkEnumeratePhysicalDevices(backend->vkInstance, &count, available);
        return
        {
            .memory = available,
            .count = count
        };
    }

    void vulkan_choose_physical_device(RendererBackendVulkan* backend)
    {
        RendererBackendVulkanArrayView<VkPhysicalDevice> available = vulkan_get_available_physical_devices(backend);
        for (uSize it = 0; it < available.count; it++)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(available.memory[it], &deviceProperties);
            if (vulkan_is_physical_device_suitable(backend, available.memory[it]))
            {
                backend->vkPhysicalDevice = available.memory[it];
                break;
            }
        }
        deallocate(&backend->bindings, available.memory, vulkan_array_view_memory_size(available));
        // @TODO :  replace default assert
        assert(backend->vkPhysicalDevice); // unable to find suitable physical device
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Logical device
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_logical_device(RendererBackendVulkan* backend)
    {
        VulkanQueueFamiliesInfo queueFamiliesInfo = vulkan_get_queue_families_info(backend, backend->vkPhysicalDevice);
        RendererBackendVulkanArrayView<VkDeviceQueueCreateInfo> queueCreateInfos = vulkan_get_queue_create_infos(backend, &queueFamiliesInfo);
        VkPhysicalDeviceFeatures deviceFeatures
        {
        };
        VkDeviceCreateInfo logicalDeviceCreateInfo
        {
            .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                      = nullptr,
            .flags                      = { },
            .queueCreateInfoCount       = queueCreateInfos.count,
            .pQueueCreateInfos          = queueCreateInfos.memory,
            // @NOTE : Validation layers info is not used by recent vulkan implemetations, but still can be set for compatibility reasons
            .enabledLayerCount          = RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS_COUNT,
            .ppEnabledLayerNames        = RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS,
            .enabledExtensionCount      = RendererBackendVulkan::REQUIRED_DEVICE_EXTENSIONS_COUNT,
            .ppEnabledExtensionNames    = RendererBackendVulkan::REQUIRED_DEVICE_EXTENSIONS,
            .pEnabledFeatures           = &deviceFeatures
        };
        al_vk_check(vkCreateDevice(backend->vkPhysicalDevice, &logicalDeviceCreateInfo, &backend->vkAllocationCallbacks, &backend->vkLogicalDevice));
        vkGetDeviceQueue(backend->vkLogicalDevice, queueFamiliesInfo.familyInfoIndex[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].index, 0, &backend->vkGraphicsQueue);
        vkGetDeviceQueue(backend->vkLogicalDevice, queueFamiliesInfo.familyInfoIndex[VulkanQueueFamiliesInfo::SURFACE_PRESENT_FAMILY].index, 0, &backend->vkPresentQueue);
        deallocate(&backend->bindings, queueCreateInfos.memory, vulkan_array_view_memory_size(queueCreateInfos));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Swap chain
    // =================================================================================================================================================
    // =================================================================================================================================================

    RendererBackendVulkanSwapChainSupportDetails vulkan_get_swap_chain_support_details(RendererBackendVulkan* backend, VkPhysicalDevice device)
    {
        RendererBackendVulkanSwapChainSupportDetails result{ };
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, backend->vkSurface, &result.capabilities);
        u32 formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, backend->vkSurface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            result.formats = 
            {
                .memory = static_cast<VkSurfaceFormatKHR*>(allocate(&backend->bindings, formatCount * sizeof(VkSurfaceFormatKHR))),
                .count = formatCount
            };
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, backend->vkSurface, &formatCount, result.formats.memory);
        }
        u32 presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, backend->vkSurface, &presentModeCount, nullptr);
        if (presentModeCount != 0)
        {
            result.presentModes =
            {
                .memory = static_cast<VkPresentModeKHR*>(allocate(&backend->bindings, presentModeCount * sizeof(VkPresentModeKHR))),
                .count = presentModeCount
            };
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, backend->vkSurface, &presentModeCount, result.presentModes.memory);
        }
        return result;
    }

    VkSurfaceFormatKHR vulkan_choose_surface_format(RendererBackendVulkanArrayView<VkSurfaceFormatKHR> availableFormats)
    {
        for (uSize it = 0; it < availableFormats.count; it++)
        {
            if (availableFormats.memory[it].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats.memory[it].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormats.memory[it];
            }
        }
        return availableFormats.memory[0];
    }

    VkPresentModeKHR vulkan_choose_presentation_mode(RendererBackendVulkanArrayView<VkPresentModeKHR> availableModes)
    {
        for (uSize it = 0; it < availableModes.count; it++)
        {
            // VK_PRESENT_MODE_MAILBOX_KHR will allow us to implemet triple buffering
            if (availableModes.memory[it] == VK_PRESENT_MODE_MAILBOX_KHR) //VK_PRESENT_MODE_IMMEDIATE_KHR
            {
                return availableModes.memory[it];
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR; // Guarateed to be available
    }

    VkExtent2D vulkan_choose_swap_extent(RendererBackendVulkan* backend, VkSurfaceCapabilitiesKHR* surfaceCapabilities)
    {
        if (surfaceCapabilities->currentExtent.width != UINT32_MAX)
        {
            return surfaceCapabilities->currentExtent;
        }
        else
        {
            auto clamp = [](u32 min, u32 max, u32 value) -> u32
            {
                return value < min ? min : (value > max ? max : value);
            };
            return
            {
                clamp(surfaceCapabilities->minImageExtent.width , surfaceCapabilities->maxImageExtent.width , platform_window_get_current_width (backend->window)),
                clamp(surfaceCapabilities->minImageExtent.height, surfaceCapabilities->maxImageExtent.height, platform_window_get_current_height(backend->window))
            };
        }
    }

    void vulkan_create_swap_chain(RendererBackendVulkan* backend)
    {
        RendererBackendVulkanSwapChainSupportDetails supportDetails = vulkan_get_swap_chain_support_details(backend, backend->vkPhysicalDevice);
        VkSurfaceFormatKHR  surfaceFormat   = vulkan_choose_surface_format     (supportDetails.formats);
        VkPresentModeKHR    presentMode     = vulkan_choose_presentation_mode  (supportDetails.presentModes);
        VkExtent2D          extent          = vulkan_choose_swap_extent        (backend, &supportDetails.capabilities);
        // @NOTE :  supportDetails.capabilities.maxImageCount == 0 means unlimited amount of images in swapchain
        u32 imageCount = supportDetails.capabilities.minImageCount + 1;
        if (supportDetails.capabilities.maxImageCount != 0 && imageCount > supportDetails.capabilities.maxImageCount)
        {
            imageCount = supportDetails.capabilities.maxImageCount;
        }
        // @NOTE :  imageSharingMode, queueFamilyIndexCount and pQueueFamilyIndices may vary depending on queue families indices.
        //          If the same queue family supports both graphics operations and presentation operations, we use exclusive sharing mode
        //          (which is more performance-friendly), but if there is two different queue families for these operations, we use
        //          concurrent sharing mode and pass indices array info to queueFamilyIndexCount and pQueueFamilyIndices
        VkSharingMode   imageSharingMode        = VK_SHARING_MODE_EXCLUSIVE;
        u32             queueFamilyIndexCount   = 0;
        const u32*      pQueueFamilyIndices     = nullptr;
        VulkanQueueFamiliesInfo queueFamiliesInfo = vulkan_get_queue_families_info(backend, backend->vkPhysicalDevice);
        u32 queueFamiliesIndices[] =
        {
            queueFamiliesInfo.familyInfoIndex[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].index,
            queueFamiliesInfo.familyInfoIndex[VulkanQueueFamiliesInfo::SURFACE_PRESENT_FAMILY].index
        };
        if (queueFamiliesIndices[0] != queueFamiliesIndices[1])
        {
            // @NOTE :  if VK_SHARING_MODE_EXCLUSIVE was used here, we would have to explicitly transfer ownership of swap chain images
            //          from one queue to another
            imageSharingMode        = VK_SHARING_MODE_CONCURRENT;
            queueFamilyIndexCount   = 2;
            pQueueFamilyIndices     = queueFamiliesIndices;
        }
        VkSwapchainCreateInfoKHR createInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext                  = nullptr,
            .flags                  = { },
            .surface                = backend->vkSurface,
            .minImageCount          = imageCount,
            .imageFormat            = surfaceFormat.format,
            .imageColorSpace        = surfaceFormat.colorSpace,
            .imageExtent            = extent,
            .imageArrayLayers       = 1, // "This is always 1 unless you are developing a stereoscopic 3D application"
            .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode       = imageSharingMode,
            .queueFamilyIndexCount  = queueFamilyIndexCount,
            .pQueueFamilyIndices    = pQueueFamilyIndices,
            .preTransform           = supportDetails.capabilities.currentTransform, // Allows to apply transform to images (rotate etc)
            .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode            = presentMode,
            .clipped                = VK_TRUE,
            .oldSwapchain           = VK_NULL_HANDLE,
        };
        al_vk_check(vkCreateSwapchainKHR(backend->vkLogicalDevice, &createInfo, &backend->vkAllocationCallbacks, &backend->vkSwapChain));
        vkGetSwapchainImagesKHR(backend->vkLogicalDevice, backend->vkSwapChain, &backend->vkSwapChainImages.count, nullptr);
        backend->vkSwapChainImages.memory = static_cast<VkImage*>(allocate(&backend->bindings, vulkan_array_view_memory_size(backend->vkSwapChainImages)));
        vkGetSwapchainImagesKHR(backend->vkLogicalDevice, backend->vkSwapChain, &backend->vkSwapChainImages.count, backend->vkSwapChainImages.memory);
        backend->vkSwapChainImageFormat = surfaceFormat.format;
        backend->vkSwapChainExtent = extent;
        deallocate(&backend->bindings, supportDetails.formats.memory, vulkan_array_view_memory_size(supportDetails.formats));
        deallocate(&backend->bindings, supportDetails.presentModes.memory, vulkan_array_view_memory_size(supportDetails.presentModes));
    }

    void vulkan_destroy_swap_chain(RendererBackendVulkan* backend)
    {
        vkDestroySwapchainKHR(backend->vkLogicalDevice, backend->vkSwapChain, &backend->vkAllocationCallbacks);
        deallocate(&backend->bindings, backend->vkSwapChainImages.memory, vulkan_array_view_memory_size(backend->vkSwapChainImages));
    }

    void vulkan_create_swap_chain_image_views(RendererBackendVulkan* backend)
    {
        backend->vkSwapChainImageViews.count = backend->vkSwapChainImages.count;
        backend->vkSwapChainImageViews.memory = static_cast<VkImageView*>(allocate(&backend->bindings, vulkan_array_view_memory_size(backend->vkSwapChainImageViews)));
        for (uSize it = 0; it < backend->vkSwapChainImageViews.count; it++)
        {
            VkImageViewCreateInfo createInfo
            {
                .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = { },
                .image              = backend->vkSwapChainImages.memory[it],
                .viewType           = VK_IMAGE_VIEW_TYPE_2D,
                .format             = backend->vkSwapChainImageFormat,
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
            al_vk_check(vkCreateImageView(backend->vkLogicalDevice, &createInfo, &backend->vkAllocationCallbacks, &backend->vkSwapChainImageViews.memory[it]));
        }
    }

    void vulkan_destroy_swap_chain_image_views(RendererBackendVulkan* backend)
    {
        for (uSize it = 0; it < backend->vkSwapChainImageViews.count; it++)
        {
            vkDestroyImageView(backend->vkLogicalDevice, backend->vkSwapChainImageViews.memory[it], &backend->vkAllocationCallbacks);
        }
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Render pass
    // =================================================================================================================================================
    // =================================================================================================================================================

    VkShaderModule vulkan_create_shader_module(RendererBackendVulkan* backend, PlatformFile spirvBytecode)
    {
        VkShaderModuleCreateInfo createInfo
        {
            .sType      = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext      = nullptr,
            .flags      = { },
            .codeSize   = spirvBytecode.sizeBytesNoAlign,
            .pCode      = static_cast<u32*>(spirvBytecode.memory),
        };
        VkShaderModule shaderModule{ };
        al_vk_check(vkCreateShaderModule(backend->vkLogicalDevice, &createInfo, &backend->vkAllocationCallbacks, &shaderModule));
        return shaderModule;
    }

    void vulkan_create_render_pass(RendererBackendVulkan* backend)
    {
        VkAttachmentDescription attachments[] = 
        {
            {   // Color attachment
                .flags          = { },
                .format         = backend->vkSwapChainImageFormat,
                .samples        = VK_SAMPLE_COUNT_1_BIT,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            }
        };
        VkAttachmentReference attachemntRefs[] =
        {
            {
                .attachment = 0, // This is the index into the VkAttachmentDescription array
                .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
        };
        VkSubpassDescription subpasses[] = 
        {
            {
                .flags                      = { },
                .pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount       = 0,
                .pInputAttachments          = nullptr,
                .colorAttachmentCount       = sizeof(attachemntRefs) / sizeof(attachemntRefs[0]),
                .pColorAttachments          = attachemntRefs,
                .pResolveAttachments        = nullptr,
                .pDepthStencilAttachment    = nullptr,
                .preserveAttachmentCount    = 0,
                .pPreserveAttachments       = nullptr,
            }
        };
        VkSubpassDependency dependencies[] = 
        {
            {
                .srcSubpass         = VK_SUBPASS_EXTERNAL,
                .dstSubpass         = 0,
                .srcStageMask       = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask       = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask      = 0,
                .dstAccessMask      = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dependencyFlags    = { },
            }
        };
        VkRenderPassCreateInfo renderPassInfo
        {
            .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = { },
            .attachmentCount    = sizeof(attachments) / sizeof(attachments[0]),
            .pAttachments       = attachments,
            .subpassCount       = sizeof(subpasses) / sizeof(subpasses[0]),
            .pSubpasses         = subpasses,
            .dependencyCount    = sizeof(dependencies) / sizeof(dependencies[0]),
            .pDependencies      = dependencies,
        };
        al_vk_check(vkCreateRenderPass(backend->vkLogicalDevice, &renderPassInfo, &backend->vkAllocationCallbacks, &backend->vkRenderPass));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Render pipeline
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_render_pipeline(RendererBackendVulkan* backend)
    {
        PlatformFile vertShader = platform_file_load(backend->bindings, RendererBackendVulkan::VERTEX_SHADER_PATH, PlatformFileLoadMode::READ);
        PlatformFile fragShader = platform_file_load(backend->bindings, RendererBackendVulkan::FRAGMENT_SHADER_PATH, PlatformFileLoadMode::READ);
        VkShaderModule vertShaderModule = vulkan_create_shader_module(backend, vertShader);
        VkShaderModule fragShaderModule = vulkan_create_shader_module(backend, fragShader);
        VkPipelineShaderStageCreateInfo shaderStages[] =
        {
            {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = { },
                .stage                  = VK_SHADER_STAGE_VERTEX_BIT,
                .module                 = vertShaderModule,
                .pName                  = "main",   // program entrypoint
                .pSpecializationInfo    = nullptr   // values for shader constants
            },
            {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = { },
                .stage                  = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module                 = fragShaderModule,
                .pName                  = "main",   // program entrypoint
                .pSpecializationInfo    = nullptr   // values for shader constants
            }
        };
        VkPipelineVertexInputStateCreateInfo vertexInputInfo
        {
            .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext                              = nullptr,
            .flags                              = { },
            .vertexBindingDescriptionCount      = 0,
            .pVertexBindingDescriptions         = nullptr,
            .vertexAttributeDescriptionCount    = 0,
            .pVertexAttributeDescriptions       = nullptr,
        };
        VkPipelineInputAssemblyStateCreateInfo inputAssembly
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = { },
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };
        VkViewport viewport
        {
            .x          = 0.0f,
            .y          = 0.0f,
            .width      = static_cast<float>(backend->vkSwapChainExtent.width),
            .height     = static_cast<float>(backend->vkSwapChainExtent.height),
            .minDepth   = 0.0f,
            .maxDepth   = 1.0f,
        };
        VkRect2D scissor
        {
            .offset = { 0, 0 },
            .extent = backend->vkSwapChainExtent,
        };
        VkPipelineViewportStateCreateInfo viewportState
        {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext          = nullptr,
            .flags          = { },
            .viewportCount  = 1,
            .pViewports     = &viewport,
            .scissorCount   = 1,
            .pScissors      = &scissor,
        };
        VkPipelineRasterizationStateCreateInfo rasterizer
        {
            .sType                      = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext                      = nullptr,
            .flags                      = { },
            .depthClampEnable           = VK_FALSE,
            .rasterizerDiscardEnable    = VK_FALSE,
            .polygonMode                = VK_POLYGON_MODE_FILL,
            .cullMode                   = VK_CULL_MODE_BACK_BIT,
            .frontFace                  = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable            = VK_FALSE,
            .depthBiasConstantFactor    = 0.0f,
            .depthBiasClamp             = 0.0f,
            .depthBiasSlopeFactor       = 0.0f,
            .lineWidth                  = 1.0f,
        };
        VkPipelineMultisampleStateCreateInfo multisampling
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = { },
            .rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable    = VK_FALSE,
            .minSampleShading       = 1.0f,
            .pSampleMask            = nullptr,
            .alphaToCoverageEnable  = VK_FALSE,
            .alphaToOneEnable       = VK_FALSE,
        };
        // VkPipelineDepthStencilStateCreateInfo{ };
        VkPipelineColorBlendAttachmentState colorBlendAttachment
        {
            .blendEnable            = VK_FALSE,
            .srcColorBlendFactor    = VK_BLEND_FACTOR_ONE,  // optional if blendEnable == VK_FALSE
            .dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO, // optional if blendEnable == VK_FALSE
            .colorBlendOp           = VK_BLEND_OP_ADD,      // optional if blendEnable == VK_FALSE
            .srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE,  // optional if blendEnable == VK_FALSE
            .dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO, // optional if blendEnable == VK_FALSE
            .alphaBlendOp           = VK_BLEND_OP_ADD,      // optional if blendEnable == VK_FALSE
            .colorWriteMask         = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
        VkPipelineColorBlendStateCreateInfo colorBlending
        {
            .sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = { },
            .logicOpEnable      = VK_FALSE,
            .logicOp            = VK_LOGIC_OP_COPY, // optional if logicOpEnable == VK_FALSE
            .attachmentCount    = 1,
            .pAttachments       = &colorBlendAttachment,
            .blendConstants     = { 0.0f, 0.0f, 0.0f, 0.0f }, // optional (???)
        };
        // @NOTE :  Example of dynamic state settings (this parameters of pipeline
        //          will be able to be changed at runtime without recreating the pipeline)
        // VkDynamicState dynamicStates[] =
        // {
        //     VK_DYNAMIC_STATE_VIEWPORT,
        //     VK_DYNAMIC_STATE_LINE_WIDTH
        // };
        // VkPipelineDynamicStateCreateInfo dynamicState
        // {
        //     .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        //     .pNext = nullptr,
        //     .flags = { },
        //     .dynamicStateCount = 2,
        //     .pDynamicStates = dynamicStates,
        // };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = { },
            .setLayoutCount         = 0,
            .pSetLayouts            = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = nullptr,
        };
        al_vk_check(vkCreatePipelineLayout(backend->vkLogicalDevice, &pipelineLayoutInfo, &backend->vkAllocationCallbacks, &backend->vkPipelineLayout));
        VkGraphicsPipelineCreateInfo pipelineInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = { },
            .stageCount             = 2,
            .pStages                = shaderStages,
            .pVertexInputState      = &vertexInputInfo,
            .pInputAssemblyState    = &inputAssembly,
            .pTessellationState     = nullptr,
            .pViewportState         = &viewportState,
            .pRasterizationState    = &rasterizer,
            .pMultisampleState      = &multisampling,
            .pDepthStencilState     = nullptr,
            .pColorBlendState       = &colorBlending,
            .pDynamicState          = nullptr,
            .layout                 = backend->vkPipelineLayout,
            .renderPass             = backend->vkRenderPass,
            .subpass                = 0,
            .basePipelineHandle     = VK_NULL_HANDLE,
            .basePipelineIndex      = -1,
        };
        al_vk_check(vkCreateGraphicsPipelines(backend->vkLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, &backend->vkAllocationCallbacks, &backend->vkGraphicsPipeline));
        vkDestroyShaderModule(backend->vkLogicalDevice, vertShaderModule, &backend->vkAllocationCallbacks);
        vkDestroyShaderModule(backend->vkLogicalDevice, fragShaderModule, &backend->vkAllocationCallbacks);
        platform_file_unload(backend->bindings, vertShader);
        platform_file_unload(backend->bindings, fragShader);
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Framebuffers
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_framebuffers(RendererBackendVulkan* backend)
    {
        backend->vkFramebuffers.count = backend->vkSwapChainImageViews.count;
        backend->vkFramebuffers.memory = static_cast<VkFramebuffer*>(allocate(&backend->bindings, vulkan_array_view_memory_size(backend->vkFramebuffers)));
        for (uSize it = 0; it < backend->vkSwapChainImageViews.count; it++)
        {
            VkImageView attachments[] =
            {
                backend->vkSwapChainImageViews.memory[it]
            };
            VkFramebufferCreateInfo framebufferInfo
            {
                .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = { },
                .renderPass         = backend->vkRenderPass,
                .attachmentCount    = sizeof(attachments) / sizeof(attachments[0]),
                .pAttachments       = attachments,
                .width              = backend->vkSwapChainExtent.width,
                .height             = backend->vkSwapChainExtent.height,
                .layers             = 1,
            };
            al_vk_check(vkCreateFramebuffer(backend->vkLogicalDevice, &framebufferInfo, &backend->vkAllocationCallbacks, &backend->vkFramebuffers.memory[it]));
        }
    }

    void vulkan_destroy_framebuffers(RendererBackendVulkan* backend)
    {
        for (uSize it = 0; it < backend->vkFramebuffers.count; it++)
        {
            vkDestroyFramebuffer(backend->vkLogicalDevice, backend->vkFramebuffers.memory[it], &backend->vkAllocationCallbacks);
        }
        deallocate(&backend->bindings, backend->vkFramebuffers.memory, vulkan_array_view_memory_size(backend->vkFramebuffers));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Command pools and command buffers
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_command_pool(RendererBackendVulkan* backend)
    {
        VulkanQueueFamiliesInfo queueFamiliesInfo = vulkan_get_queue_families_info(backend, backend->vkPhysicalDevice);
        VkCommandPoolCreateInfo poolInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = { },
            .queueFamilyIndex   = queueFamiliesInfo.familyInfoIndex[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].index
        };
        al_vk_check(vkCreateCommandPool(backend->vkLogicalDevice, &poolInfo, &backend->vkAllocationCallbacks, &backend->vkCommandPool));

    }

    void vulkan_destroy_command_pool(RendererBackendVulkan* backend)
    {
        vkDestroyCommandPool(backend->vkLogicalDevice, backend->vkCommandPool, &backend->vkAllocationCallbacks);
    }

    void vulkan_create_command_buffers(RendererBackendVulkan* backend)
    {
        // @NOTE :  creating command buffers for each possible swap chain image
        backend->vkCommandBuffers.count = backend->vkSwapChainImageViews.count;
        backend->vkCommandBuffers.memory = static_cast<VkCommandBuffer*>(allocate(&backend->bindings, vulkan_array_view_memory_size(backend->vkCommandBuffers)));
        VkCommandBufferAllocateInfo allocInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = backend->vkCommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            // @TODO :  safe-cast from size_t to u32
            .commandBufferCount = static_cast<u32>(backend->vkCommandBuffers.count),
        };
        al_vk_check(vkAllocateCommandBuffers(backend->vkLogicalDevice, &allocInfo, backend->vkCommandBuffers.memory));
        // @NOTE :  pre-recording command buffer for drawing a triangle
        for (uSize it = 0; it < backend->vkCommandBuffers.count; it++) {
            VkCommandBuffer commandBuffer = backend->vkCommandBuffers.memory[it];
            VkCommandBufferBeginInfo beginInfo
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext              = nullptr,
                .flags              = { },
                .pInheritanceInfo   = nullptr,  // relevant only for secondary command buffers
            };
            al_vk_check(vkBeginCommandBuffer(commandBuffer, &beginInfo));
            VkClearValue clearValue
            {
                .color = { 0.0f, 0.0f, 0.0f, 1.0f }
            };
            VkRenderPassBeginInfo renderPassInfo
            {
                .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext              = nullptr,
                .renderPass         = backend->vkRenderPass,
                .framebuffer        = backend->vkFramebuffers.memory[it],
                .renderArea         = { .offset = { 0, 0 }, .extent = backend->vkSwapChainExtent },
                .clearValueCount    = 1,
                .pClearValues       = &clearValue,
            };
            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, backend->vkGraphicsPipeline);
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffer);
            al_vk_check(vkEndCommandBuffer(commandBuffer));
        }
    }

    void vulkan_destroy_command_buffers(RendererBackendVulkan* backend)
    {
        // Nothing here? (command buffers gets automatically deleted with command pools)
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Synchronization primitives
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_semaphores(RendererBackendVulkan* backend)
    {
        backend->vkImageAvailableSemaphores.count = RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT;
        backend->vkImageAvailableSemaphores.memory = static_cast<VkSemaphore*>(allocate(&backend->bindings, vulkan_array_view_memory_size(backend->vkImageAvailableSemaphores)));
        backend->vkRenderFinishedSemaphores.count = RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT;
        backend->vkRenderFinishedSemaphores.memory = static_cast<VkSemaphore*>(allocate(&backend->bindings, vulkan_array_view_memory_size(backend->vkRenderFinishedSemaphores)));
        for (uSize it = 0; it < RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT; it++)
        {
            VkSemaphoreCreateInfo semaphoreInfo
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = { },
            };
            al_vk_check(vkCreateSemaphore(backend->vkLogicalDevice, &semaphoreInfo, &backend->vkAllocationCallbacks, &backend->vkImageAvailableSemaphores.memory[it]));
            al_vk_check(vkCreateSemaphore(backend->vkLogicalDevice, &semaphoreInfo, &backend->vkAllocationCallbacks, &backend->vkRenderFinishedSemaphores.memory[it]));
        }
    }

    void vulkan_destroy_semaphores(RendererBackendVulkan* backend)
    {
        for (uSize it = 0; it < RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT; it++)
        {
            vkDestroySemaphore(backend->vkLogicalDevice, backend->vkImageAvailableSemaphores.memory[it], &backend->vkAllocationCallbacks);
            vkDestroySemaphore(backend->vkLogicalDevice, backend->vkRenderFinishedSemaphores.memory[it], &backend->vkAllocationCallbacks);
        }
        deallocate(&backend->bindings, backend->vkImageAvailableSemaphores.memory, vulkan_array_view_memory_size(backend->vkImageAvailableSemaphores));
        deallocate(&backend->bindings, backend->vkRenderFinishedSemaphores.memory, vulkan_array_view_memory_size(backend->vkRenderFinishedSemaphores));
    }

    void vulkan_create_fences(RendererBackendVulkan* backend)
    {
        backend->vkInFlightFences.count = RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT;
        backend->vkInFlightFences.memory = static_cast<VkFence*>(allocate(&backend->bindings, vulkan_array_view_memory_size(backend->vkInFlightFences)));
        for (uSize it = 0; it < RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT; it++)
        {
            VkFenceCreateInfo fenceInfo
            {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };
            al_vk_check(vkCreateFence(backend->vkLogicalDevice, &fenceInfo, &backend->vkAllocationCallbacks, &backend->vkInFlightFences.memory[it]));
        }
        backend->vkImageInFlightFences.count = backend->vkSwapChainImages.count;
        backend->vkImageInFlightFences.memory = static_cast<VkFence*>(allocate(&backend->bindings, vulkan_array_view_memory_size(backend->vkImageInFlightFences)));
        for (uSize it = 0; it < backend->vkImageInFlightFences.count; it++)
        {
            backend->vkImageInFlightFences.memory[it] = VK_NULL_HANDLE;
        }
    }

    void vulkan_destroy_fences(RendererBackendVulkan* backend)
    {
        for (uSize it = 0; it < RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT; it++)
        {
            vkDestroyFence(backend->vkLogicalDevice, backend->vkInFlightFences.memory[it], &backend->vkAllocationCallbacks);
        }
        // @NOTE :  fences from vkImageInFlightFences doesn't need to be destroyed because they reference already destroyed fences from vkInFlightFences
        deallocate(&backend->bindings, backend->vkInFlightFences.memory, vulkan_array_view_memory_size(backend->vkInFlightFences));
        deallocate(&backend->bindings, backend->vkImageInFlightFences.memory, vulkan_array_view_memory_size(backend->vkImageInFlightFences));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Renderer backend interface
    // =================================================================================================================================================
    // =================================================================================================================================================

    void renderer_backend_construct(RendererBackendVulkan* backend, RendererBackendInitData* initData)
    {
        // Shitty test vulkan implementation

        log_msg("Constructing vulkan backend\n");

        backend->bindings = initData->bindings;
        backend->window = initData->window;
        vulkan_set_allocation_callbacks         (backend);
        vulkan_create_vk_instance               (backend, initData);
        vulkan_setup_debug_messenger            (backend);
        vulkan_create_surface                   (backend);
        vulkan_choose_physical_device           (backend);
        vulkan_create_logical_device            (backend);
        vulkan_create_swap_chain                (backend);
        vulkan_create_swap_chain_image_views    (backend);
        vulkan_create_render_pass               (backend);
        vulkan_create_render_pipeline           (backend);
        vulkan_create_framebuffers              (backend);
        vulkan_create_command_pool              (backend);
        vulkan_create_command_buffers           (backend);
        vulkan_create_semaphores                (backend);
        vulkan_create_fences                    (backend);
    }

    void renderer_backend_render(RendererBackendVulkan* backend)
    {
        uSize currentFrame = backend->currentFrameNumber % RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT;
        backend->currentFrameNumber = currentFrame;

        // @NOTE :  wait for fence of current frame
        vkWaitForFences(backend->vkLogicalDevice, 1, &backend->vkInFlightFences.memory[currentFrame], VK_TRUE, UINT64_MAX);

        u32 imageIndex;
        // @NOTE :  timeout value is in nanoseconds. Using UINT64_MAX disables timeout
        vkAcquireNextImageKHR(backend->vkLogicalDevice, backend->vkSwapChain, UINT64_MAX, backend->vkImageAvailableSemaphores.memory[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // @NOTE :  must check for current swap chain image fence (commands for this image might still be executing at this point -
        //          this can happen only if RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT is bigger than number of swap chain images)
        if (backend->vkImageInFlightFences.memory[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(backend->vkLogicalDevice, 1, &backend->vkImageInFlightFences.memory[imageIndex], VK_TRUE, UINT64_MAX);
        }
        // @NOTE :  this marks this image as occupied by this frame
        backend->vkImageInFlightFences.memory[imageIndex] = backend->vkInFlightFences.memory[currentFrame];

        VkSemaphore waitSemaphores[] =
        {
            backend->vkImageAvailableSemaphores.memory[currentFrame]
        };
        VkPipelineStageFlags waitStages[] =
        {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        VkSemaphore signalSemaphores[] =
        {
            backend->vkRenderFinishedSemaphores.memory[currentFrame]
        };
        VkSubmitInfo submitInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                  = nullptr,
            .waitSemaphoreCount     = sizeof(waitSemaphores) / sizeof(waitSemaphores[0]),
            .pWaitSemaphores        = waitSemaphores,
            .pWaitDstStageMask      = waitStages,
            .commandBufferCount     = 1,
            .pCommandBuffers        = &backend->vkCommandBuffers.memory[imageIndex],
            .signalSemaphoreCount   = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]),
            .pSignalSemaphores      = signalSemaphores,
        };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        vkResetFences(backend->vkLogicalDevice, 1, &backend->vkInFlightFences.memory[currentFrame]);
        vkQueueSubmit(backend->vkGraphicsQueue, 1, &submitInfo, backend->vkInFlightFences.memory[currentFrame]);
        VkSwapchainKHR swapChains[] =
        {
            backend->vkSwapChain
        };
        VkPresentInfoKHR presentInfo
        {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext              = nullptr,
            .waitSemaphoreCount = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]),
            .pWaitSemaphores    = signalSemaphores,
            .swapchainCount     = sizeof(swapChains) / sizeof(swapChains[0]),
            .pSwapchains        = swapChains,
            .pImageIndices      = &imageIndex,
            .pResults           = nullptr,
        };
        vkQueuePresentKHR(backend->vkPresentQueue, &presentInfo);
        // vkQueueWaitIdle(backend->vkPresentQueue);
    }

    void renderer_backend_destruct(RendererBackendVulkan* backend)
    {
        log_msg("Destructing vulkan backend\n");

        // @NOTE :  wait for all commands to finish execution
        vkDeviceWaitIdle(backend->vkLogicalDevice);

        vulkan_destroy_fences                   (backend);
        vulkan_destroy_semaphores               (backend);
        vulkan_destroy_command_buffers          (backend);
        vulkan_destroy_command_pool             (backend);
        vulkan_destroy_framebuffers             (backend);
        vkDestroyPipeline                       (backend->vkLogicalDevice, backend->vkGraphicsPipeline, &backend->vkAllocationCallbacks);
        vkDestroyPipelineLayout                 (backend->vkLogicalDevice, backend->vkPipelineLayout, &backend->vkAllocationCallbacks);
        vkDestroyRenderPass                     (backend->vkLogicalDevice, backend->vkRenderPass, &backend->vkAllocationCallbacks);
        vulkan_destroy_swap_chain_image_views   (backend);
        vulkan_destroy_swap_chain               (backend);
        vkDestroyDevice                         (backend->vkLogicalDevice, &backend->vkAllocationCallbacks);
        vkDestroySurfaceKHR                     (backend->vkInstance, backend->vkSurface, &backend->vkAllocationCallbacks);
        vulkan_destroy_debug_messenger          (backend);
        vkDestroyInstance                       (backend->vkInstance, &backend->vkAllocationCallbacks);
    }
}
