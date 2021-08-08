
#include <type_traits>

#include "vulkan_utils.h"

namespace al::utils
{
    PointerWithSize<const char*> get_required_validation_layers()
    {
        static const char* VALIDATION_LAYERS[] = 
        {
            "VK_LAYER_KHRONOS_validation",
        };
        return { VALIDATION_LAYERS, array_size(VALIDATION_LAYERS) };
    }

    PointerWithSize<const char*> get_required_instance_extensions()
    {
        static const char* WINDOWS_INSTANCE_EXTENSIONS[] = 
        {
            "VK_KHR_surface",
            "VK_KHR_win32_surface",
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        };
        return { WINDOWS_INSTANCE_EXTENSIONS, array_size(WINDOWS_INSTANCE_EXTENSIONS) };
    }

    PointerWithSize<const char*> get_required_device_extensions()
    {
        static const char* DEVICE_EXTENSIONS[] = 
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        return { DEVICE_EXTENSIONS, array_size(DEVICE_EXTENSIONS) };
    }
    
    Array<VkLayerProperties> get_available_validation_layers(AllocatorBindings* bindings)
    {
        u32 count;
        Array<VkLayerProperties> result;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        array_construct(&result, bindings, count);
        vkEnumerateInstanceLayerProperties(&count, result.memory);
        return result;
    }

    Array<VkExtensionProperties> get_available_instance_extensions(AllocatorBindings* bindings)
    {
        u32 count;
        Array<VkExtensionProperties> result;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        array_construct(&result, bindings, count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, result.memory);
        return result;
    }

    VkDebugUtilsMessengerCreateInfoEXT get_debug_messenger_create_info(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* userData)
    {
        return
        {
            .sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext              = nullptr,
            .flags              = 0,
            .messageSeverity    = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType        = /*VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback    = callback,
            .pUserData          = userData,
        };
    }

    VkDebugUtilsMessengerEXT create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT* createInfo, VkInstance instance, VkAllocationCallbacks* callbacks)
    {
        VkDebugUtilsMessengerEXT messenger;
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
        al_vk_check(CreateDebugUtilsMessengerEXT(instance, createInfo, callbacks, &messenger));
        return messenger;
    }

    void destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks* callbacks)
    {
        auto DestroyDebugUtilsMessengerEXT = [](VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
        {
            PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func)
            {
                func(instance, messenger, pAllocator);
            }
        };
        DestroyDebugUtilsMessengerEXT(instance, messenger, callbacks);
    }

    VkCommandPool create_command_pool(VkDevice device, u32 queueFamilyIndex, VkAllocationCallbacks* callbacks, VkCommandPoolCreateFlags flags)
    {
        VkCommandPool pool;
        VkCommandPoolCreateInfo poolInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = flags,
            .queueFamilyIndex   = queueFamilyIndex,
        };
        al_vk_check(vkCreateCommandPool(device, &poolInfo, callbacks, &pool));
        return pool;
    }

    void destroy_command_pool(VkCommandPool pool, VkDevice device, VkAllocationCallbacks* callbacks)
    {
        vkDestroyCommandPool(device, pool, callbacks);
    }

    SwapChainSupportDetails create_swap_chain_support_details(VkSurfaceKHR surface, VkPhysicalDevice device, AllocatorBindings* bindings)
    {
        SwapChainSupportDetails result{ };
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &result.capabilities);
        u32 count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
        if (count != 0)
        {
            array_construct(&result.formats, bindings, count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, result.formats.memory);
        }
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
        if (count != 0)
        {
            array_construct(&result.presentModes, bindings, count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, result.presentModes.memory);
        }
        return result;
    }

    void destroy_swap_chain_support_details(SwapChainSupportDetails* details)
    {
        array_destruct(&details->formats);
        array_destruct(&details->presentModes);
    }

    VkSurfaceFormatKHR choose_swap_chain_surface_format(Array<VkSurfaceFormatKHR> available)
    {
        for (uSize it = 0; it < available.size; it++)
        {
            if (available[it].format == /*VK_FORMAT_B8G8R8A8_SRGB*/ VK_FORMAT_R8G8B8A8_SRGB && available[it].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return available[it];
            }
        }
        return available[0];
    }

    VkPresentModeKHR choose_swap_chain_surface_present_mode(Array<VkPresentModeKHR> available)
    {
        for (uSize it = 0; it < available.size; it++)
        {
            if (available[it] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return available[it];
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR; // Guarateed to be available
    }

    VkExtent2D choose_swap_chain_extent(u32 windowWidth, u32 windowHeight, VkSurfaceCapabilitiesKHR* capabilities)
    {
        if (capabilities->currentExtent.width != UINT32_MAX)
        {
            return capabilities->currentExtent;
        }
        else
        {
            auto clamp = [](u32 min, u32 max, u32 value) -> u32 { return value < min ? min : (value > max ? max : value); };
            return
            {
                clamp(capabilities->minImageExtent.width , capabilities->maxImageExtent.width , windowWidth),
                clamp(capabilities->minImageExtent.height, capabilities->maxImageExtent.height, windowHeight)
            };
        }
    }

    Tuple<bool, u32> pick_graphics_queue(Array<VkQueueFamilyProperties> familyProperties)
    {
        for (u32 it = 0; it < familyProperties.size; it++)
        {
            if (familyProperties[it].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                return { true, it };
            }
        }
        return { false, 0 };
    }

    Tuple<bool, u32> pick_present_queue(Array<VkQueueFamilyProperties> familyProperties, VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        for (u32 it = 0; it < familyProperties.size; it++)
        {
            VkBool32 isSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, it, surface, &isSupported);
            if (isSupported)
            {
                return { true, it };
            }
        }
        return { false, 0 };
    }

    Tuple<bool, u32> pick_transfer_queue(Array<VkQueueFamilyProperties> familyProperties)
    {
        for (u32 it = 0; it < familyProperties.size; it++)
        {
            if (familyProperties[it].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                return { true, it };
            }
        }
        return { false, 0 };
    }

    template<uSize Size>
    Array<VkDeviceQueueCreateInfo> get_queue_create_infos(StaticPointerWithSize<Tuple<bool, u32>, Size> queues, AllocatorBindings* bindings)
    {
        // @NOTE :  this is possible that queue family might support more than one of the required features,
        //          so we have to remove duplicates from queueFamiliesInfo and create VkDeviceQueueCreateInfos
        //          only for unique indexes
        static const f32 QUEUE_DEFAULT_PRIORITY = 1.0f;
        u32 indicesArray[Size];
        PointerWithSize<u32> uniqueQueueIndices
        {
            .ptr = indicesArray,
            .size = 0
        };
        auto updateUniqueIndicesArray = [](PointerWithSize<u32>* targetArray, u32 familyIndex)
        {
            bool isFound = false;
            for (uSize uniqueIt = 0; uniqueIt < targetArray->size; uniqueIt++)
            {
                if ((*targetArray)[uniqueIt] == familyIndex)
                {
                    isFound = true;
                    break;
                }
            }
            if (!isFound)
            {
                targetArray->size += 1;
                (*targetArray)[targetArray->size - 1] = familyIndex;
            }
        };
        for (uSize it = 0; it < Size; it++)
        {
            updateUniqueIndicesArray(&uniqueQueueIndices, get<1>(queues[it]));
        }
        Array<VkDeviceQueueCreateInfo> result;
        array_construct(&result, bindings, uniqueQueueIndices.size);
        for (uSize it = 0; it < result.size; it++)
        {
            result[it] = 
            {
                .sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = 0,
                .queueFamilyIndex   = uniqueQueueIndices[it],
                .queueCount         = 1,
                .pQueuePriorities   = &QUEUE_DEFAULT_PRIORITY,
            };
        }
        return result;
    }

    bool pick_depth_stencil_format(VkPhysicalDevice physicalDevice, VkFormat* result)
    {
        // @NOTE :  taken from https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
        // Since all depth formats may be optional, we need to find a suitable depth format to use
        // Start with the highest precision packed format
        VkFormat depthStecilFormats[] =
        {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D16_UNORM,
        };
        for (uSize it = 0; it < array_size(depthStecilFormats); it++)
        {
            VkFormat format = depthStecilFormats[it];
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
            // Format must support depth stencil attachment for optimal tiling
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                *result = format;
                return it < 3;
            }
        }
        return false;
    }

    Array<VkPhysicalDevice> get_available_physical_devices(VkInstance instance, AllocatorBindings* bindings)
    {
        u32 count;
        Array<VkPhysicalDevice> result;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        array_construct(&result, bindings, count);
        vkEnumeratePhysicalDevices(instance, &count, result.memory);
        return result;
    };

    Array<VkQueueFamilyProperties> get_physical_device_queue_family_properties(VkPhysicalDevice physicalDevice, AllocatorBindings* bindings)
    {
        u32 count;
        Array<VkQueueFamilyProperties> familyProperties;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, nullptr);
        array_construct(&familyProperties, bindings, count);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, familyProperties.memory);
        return familyProperties;
    }

    bool does_physical_device_supports_required_extensions(VkPhysicalDevice device, PointerWithSize<const char*> extensions, AllocatorBindings* bindings)
    {
        u32 count;
        VkPhysicalDeviceFeatures feat{};
        vkGetPhysicalDeviceFeatures(device, &feat);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
        Array<VkExtensionProperties> availableExtensions;
        array_construct(&availableExtensions, bindings, count);
        defer(array_destruct(&availableExtensions));
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, availableExtensions.memory);
        bool isRequiredExtensionAvailable;
        bool result = true;
        for (uSize requiredIt = 0; requiredIt < extensions.size; requiredIt++)
        {
            isRequiredExtensionAvailable = false;
            for (uSize availableIt = 0; availableIt < availableExtensions.size; availableIt++)
            {
                if (al_vk_strcmp(availableExtensions[availableIt].extensionName, extensions[requiredIt]))
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
        return result;
    };

    bool does_physical_device_supports_required_features(VkPhysicalDevice device, VkPhysicalDeviceFeatures* requiredFeatures)
    {
        // VkPhysicalDeviceFeatures is just a collection of VkBool32 values, so we can iterate over it like an array
        VkPhysicalDeviceFeatures supportedFeatures{ };
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
        VkBool32* requiredArray = reinterpret_cast<VkBool32*>(requiredFeatures);
        VkBool32* supportedArray = reinterpret_cast<VkBool32*>(&supportedFeatures);
        for (uSize it = 0; it < sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32); it++)
        {
            if (!(!requiredArray[it] || supportedArray[it]))
            {
                return false;
            }
        }
        return true;
    }

    VkImageType pick_image_type(VkExtent3D imageExtent)
    {
        // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkImageCreateInfo.html
        // If imageType is VK_IMAGE_TYPE_1D, both extent.height and extent.depth must be 1
        if (imageExtent.height == 1 && imageExtent.depth == 1)
        {
            return VK_IMAGE_TYPE_1D;
        }
        // If imageType is VK_IMAGE_TYPE_2D, extent.depth must be 1
        if (imageExtent.depth == 1)
        {
            return VK_IMAGE_TYPE_2D;
        }
        return VK_IMAGE_TYPE_3D;
    }

    VkCommandBuffer create_command_buffer(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level)
    {
        VkCommandBuffer buffer;
        VkCommandBufferAllocateInfo allocInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = pool,
            .level              = level,
            .commandBufferCount = 1,
        };
        al_vk_check(vkAllocateCommandBuffers(device, &allocInfo, &buffer));
        return buffer;
    };

    VkShaderModule create_shader_module(VkDevice device, Tuple<u32*, uSize> bytecode, VkAllocationCallbacks* allocationCb)
    {
        VkShaderModuleCreateInfo createInfo
        {
            .sType      = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext      = nullptr,
            .flags      = 0,
            .codeSize   = get<1>(bytecode),
            .pCode      = get<0>(bytecode),
        };
        VkShaderModule shaderModule;
        al_vk_check(vkCreateShaderModule(device, &createInfo, allocationCb, &shaderModule));
        return shaderModule;
    }

    void destroy_shader_module(VkDevice device, VkShaderModule module, VkAllocationCallbacks* allocationCb)
    {
        vkDestroyShaderModule(device, module, allocationCb);
    }

    bool get_memory_type_index(VkPhysicalDeviceMemoryProperties* props, u32 typeBits, VkMemoryPropertyFlags properties, u32* result)
    {
        for (u32 it = 0; it < props->memoryTypeCount; it++)
        {
            if (typeBits & 1)
            {
                if ((props->memoryTypes[it].propertyFlags & properties) == properties)
                {
                    *result = it;
                    return true;
                }
            }
            typeBits >>= 1;
        }
        return false;
    }

    TextureFormat to_texture_format(VkFormat vkFormat)
    {
        switch (vkFormat)
        {
            case VK_FORMAT_R8G8B8A8_SRGB: return TextureFormat::RGBA_8;
            case VK_FORMAT_R32G32B32A32_SFLOAT: return TextureFormat::RGBA_32F;
        }
        al_vk_assert_fail("Unsupported VkFormat");
        return TextureFormat(0);
    }

    VkFormat to_vk_format(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::RGBA_8: return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureFormat::RGBA_32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        al_vk_assert_fail("Unsupported TextureFormat");
        return VkFormat(0);
    }

    VkAttachmentLoadOp to_vk_load_op(AttachmentLoadOp loadOp)
    {
        switch (loadOp)
        {
            case AttachmentLoadOp::CLEAR: return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case AttachmentLoadOp::LOAD: return VK_ATTACHMENT_LOAD_OP_LOAD;
            case AttachmentLoadOp::NOTHING: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        al_vk_assert_fail("Unsupported FramebufferAttachmentLoadOp");
        return VkAttachmentLoadOp(0);
    }

    VkAttachmentStoreOp to_vk_store_op(AttachmentStoreOp storeOp)
    {
        switch (storeOp)
        {
            case AttachmentStoreOp::STORE: return VK_ATTACHMENT_STORE_OP_STORE;
            case AttachmentStoreOp::NOTHING: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        al_vk_assert_fail("Unsupported FramebufferAttachmentStoreOp");
        return VkAttachmentStoreOp(0);
    }
}
