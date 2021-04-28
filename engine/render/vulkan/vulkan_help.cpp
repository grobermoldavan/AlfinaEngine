
#include "vulkan_help.h"

namespace al::vk
{
    ArrayView<VkLayerProperties> get_available_validation_layers(AllocatorBindings* bindings)
    {
        u32 count;
        ArrayView<VkLayerProperties> result;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        av_construct(&result, bindings, count);
        vkEnumerateInstanceLayerProperties(&count, result.memory);
        return result;
    }

    ArrayView<VkExtensionProperties> get_available_instance_extensions(AllocatorBindings* bindings)
    {
        u32 count;
        ArrayView<VkExtensionProperties> result;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        av_construct(&result, bindings, count);
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
            .messageType        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback    = callback,
            .pUserData          = userData,
        };
    }

    VkDebugUtilsMessengerEXT create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT* createInfo, VkInstance instance, VkAllocationCallbacks* allocationCb)
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
        al_vk_check(CreateDebugUtilsMessengerEXT(instance, createInfo, allocationCb, &messenger));
        return messenger;
    }

    bool pick_depth_format(VkPhysicalDevice physicalDevice, VkFormat* result)
    {
        // @NOTE :  taken from https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanTools.cpp
        // Since all depth formats may be optional, we need to find a suitable depth format to use
        // Start with the highest precision packed format
        VkFormat depthFormats[] =
        {
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D24_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM_S8_UINT,
            VK_FORMAT_D16_UNORM
        };
        for (uSize it = 0; it < array_size(depthFormats); it++)
        {
            VkFormat format = depthFormats[it];
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
            // Format must support depth stencil attachment for optimal tiling
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                *result = format;
                return true;
            }
        }
        return false;
    }

    ArrayView<VkPhysicalDevice> get_available_physical_devices(VkInstance instance, AllocatorBindings bindings)
    {
        u32 count;
        ArrayView<VkPhysicalDevice> result;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        av_construct(&result, &bindings, count);
        vkEnumeratePhysicalDevices(instance, &count, result.memory);
        return result;
    };

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

    uSize advance_render_frame(uSize previousRenderFrame)
    {
        return (previousRenderFrame + 1) % MAX_IMAGES_IN_FLIGHT;
    }
}
