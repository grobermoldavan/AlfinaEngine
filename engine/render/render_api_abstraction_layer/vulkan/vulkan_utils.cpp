
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
    Array<VkDeviceQueueCreateInfo> get_queue_create_infos(Tuple<bool, u32> (&queues)[Size], AllocatorBindings* bindings)
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
        for (al_iterator(it, queues))
        {
            updateUniqueIndicesArray(&uniqueQueueIndices, get<1>(*get(it)));
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

    VkCommandPoolCreateInfo command_pool_create_info(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags)
    {
        return
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = flags,
            .queueFamilyIndex   = queueFamilyIndex,
        };
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
        al_assert_fail("Unsupported VkFormat");
        return TextureFormat(0);
    }

    VkFormat to_vk_format(TextureFormat format)
    {
        switch (format)
        {
            case TextureFormat::RGBA_8: return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureFormat::RGBA_32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        al_assert_fail("Unsupported TextureFormat");
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
        al_assert_fail("Unsupported FramebufferAttachmentLoadOp");
        return VkAttachmentLoadOp(0);
    }

    VkAttachmentStoreOp to_vk_store_op(AttachmentStoreOp storeOp)
    {
        switch (storeOp)
        {
            case AttachmentStoreOp::STORE: return VK_ATTACHMENT_STORE_OP_STORE;
            case AttachmentStoreOp::NOTHING: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }
        al_assert_fail("Unsupported FramebufferAttachmentStoreOp");
        return VkAttachmentStoreOp(0);
    }

    VkPolygonMode to_vk_polygon_mode(PipelinePoligonMode mode)
    {
        switch (mode)
        {
            case PipelinePoligonMode::FILL: return VK_POLYGON_MODE_FILL;
            case PipelinePoligonMode::LINE: return VK_POLYGON_MODE_LINE;
            case PipelinePoligonMode::POINT: return VK_POLYGON_MODE_POINT;
        }
        al_assert_fail("Unsupported PipelinePoligonMode");
        return VkPolygonMode(0);
    }

    VkCullModeFlags to_vk_cull_mode(PipelineCullMode mode)
    {
        switch (mode)
        {
            case PipelineCullMode::NONE: return VK_CULL_MODE_NONE;
            case PipelineCullMode::FRONT: return VK_CULL_MODE_FRONT_BIT;
            case PipelineCullMode::BACK: return VK_CULL_MODE_BACK_BIT;
            case PipelineCullMode::FRONT_BACK: return VK_CULL_MODE_FRONT_AND_BACK;
        }
        al_assert_fail("Unsupported PipelineCullMode");
        return VkCullModeFlags(0);
    }

    VkFrontFace to_vk_front_face(PipelineFrontFace frontFace)
    {
        switch (frontFace)
        {
            case PipelineFrontFace::CLOCKWISE: return VK_FRONT_FACE_CLOCKWISE;
            case PipelineFrontFace::COUNTER_CLOCKWISE: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }
        al_assert_fail("Unsupported PipelineFrontFace");
        return VkFrontFace(0);
    }
    
    VkSampleCountFlagBits to_vk_sample_count(MultisamplingType multisample)
    {
        switch (multisample)
        {
            case MultisamplingType::SAMPLE_1: return VK_SAMPLE_COUNT_1_BIT;
            case MultisamplingType::SAMPLE_2: return VK_SAMPLE_COUNT_2_BIT;
            case MultisamplingType::SAMPLE_4: return VK_SAMPLE_COUNT_4_BIT;
            case MultisamplingType::SAMPLE_8: return VK_SAMPLE_COUNT_8_BIT;
        }
        al_assert_fail("Unsupported MultisamplingType");
        return VkSampleCountFlagBits(0);
    }

    VkSampleCountFlagBits pick_sample_count(VkSampleCountFlags desired, VkSampleCountFlags supported)
    {
        if (supported & desired) return VkSampleCountFlagBits(desired);
        for (VkSampleCountFlags it = sizeof(VkSampleCountFlags) * 8 - 1; it >= 0; it++)
        {
            if (((supported >> it) & 1) && it < desired)
            {
                return VkSampleCountFlagBits(it);
            }
        }
        al_assert_fail("Unable to pick sample count");
        return VkSampleCountFlagBits(0);
    }

    VkStencilOp to_vk_stencil_op(StencilOp op)
    {
        switch (op)
        {
            case StencilOp::KEEP:                   return VK_STENCIL_OP_KEEP;
            case StencilOp::ZERO:                   return VK_STENCIL_OP_ZERO;
            case StencilOp::REPLACE:                return VK_STENCIL_OP_REPLACE;
            case StencilOp::INCREMENT_AND_CLAMP:    return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
            case StencilOp::DECREMENT_AND_CLAMP:    return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
            case StencilOp::INVERT:                 return VK_STENCIL_OP_INVERT;
            case StencilOp::INCREMENT_AND_WRAP:     return VK_STENCIL_OP_INCREMENT_AND_WRAP;
            case StencilOp::DECREMENT_AND_WRAP:     return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        }
        al_assert_fail("Unsupported StencilOp");
        return VkStencilOp(0);
    }

    VkCompareOp to_vk_compare_op(CompareOp op)
    {
        switch (op)
        {
            case CompareOp::NEVER:              return VK_COMPARE_OP_NEVER;
            case CompareOp::LESS:               return VK_COMPARE_OP_LESS;
            case CompareOp::EQUAL:              return VK_COMPARE_OP_EQUAL;
            case CompareOp::LESS_OR_EQUAL:      return VK_COMPARE_OP_LESS_OR_EQUAL;
            case CompareOp::GREATER:            return VK_COMPARE_OP_GREATER;
            case CompareOp::NOT_EQUAL:          return VK_COMPARE_OP_NOT_EQUAL;
            case CompareOp::GREATER_OR_EQUAL:   return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case CompareOp::ALWAYS:             return VK_COMPARE_OP_ALWAYS;
        }
        al_assert_fail("Unsupported CompareOp");
        return VkCompareOp(0);
    }

    VkBool32 to_vk_bool(bool value)
    {
        return value ? VK_TRUE : VK_FALSE;
    }

    VkDescriptorType to_vk_descriptor_type(SpirvReflection::Uniform::Type type)
    {
        switch(type)
        {
            case SpirvReflection::Uniform::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
            case SpirvReflection::Uniform::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case SpirvReflection::Uniform::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case SpirvReflection::Uniform::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case SpirvReflection::Uniform::UniformTexelBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            case SpirvReflection::Uniform::StorageTexelBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            case SpirvReflection::Uniform::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case SpirvReflection::Uniform::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case SpirvReflection::Uniform::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            case SpirvReflection::Uniform::AccelerationStructure: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        }
        return VkDescriptorType(0);
    }

    VkShaderStageFlags to_vk_stage_flags(ProgramStageFlags stages)
    {
        return
            VkShaderStageFlags(stages & u32(ProgramStage::Vertex)   ? VK_SHADER_STAGE_VERTEX_BIT    : 0) |
            VkShaderStageFlags(stages & u32(ProgramStage::Fragment) ? VK_SHADER_STAGE_FRAGMENT_BIT  : 0) |
            VkShaderStageFlags(stages & u32(ProgramStage::Compute)  ? VK_SHADER_STAGE_COMPUTE_BIT   : 0);
    }

    VkPipelineShaderStageCreateInfo shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule module, const char* pName)
    {
        return
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .stage                  = stage,
            .module                 = module,
            .pName                  = pName,
            .pSpecializationInfo    = nullptr,
        };
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info(u32 bindingsCount, const VkVertexInputBindingDescription* bindingDescs, u32 attrCount, const VkVertexInputAttributeDescription* attrDescs)
    {
        return
        {
            .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext                              = nullptr,
            .flags                              = 0,
            .vertexBindingDescriptionCount      = bindingsCount,
            .pVertexBindingDescriptions         = bindingDescs,
            .vertexAttributeDescriptionCount    = attrCount,
            .pVertexAttributeDescriptions       = attrDescs,
        };
    }

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable)
    {
        return
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .topology               = topology,
            .primitiveRestartEnable = primitiveRestartEnable,
        };
    }

    ViewportScissor default_viewport_scissor(u32 width, u32 height)
    {
        return
        {
            {
                .x          = 0.0f,
                .y          = 0.0f,
                .width      = (float)width,
                .height     = (float)height,
                .minDepth   = 0.0f,
                .maxDepth   = 1.0f,
            },
            {
                .offset = { 0, 0 },
                .extent = { width, height },
            }
        };
    }

    VkPipelineViewportStateCreateInfo viewport_state_create_info(VkViewport* viewport, VkRect2D* scissor)
    {
        return
        {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext          = nullptr,
            .flags          = 0,
            .viewportCount  = 1,
            .pViewports     = viewport,
            .scissorCount   = 1,
            .pScissors      = scissor,
        };
    }

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace)
    {
        return
        {
            .sType                      = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext                      = nullptr,
            .flags                      = 0,
            .depthClampEnable           = VK_FALSE,
            .rasterizerDiscardEnable    = VK_FALSE,
            .polygonMode                = polygonMode,
            .cullMode                   = cullMode,
            .frontFace                  = frontFace,
            .depthBiasEnable            = VK_FALSE,
            .depthBiasConstantFactor    = 0.0f,
            .depthBiasClamp             = 0.0f,
            .depthBiasSlopeFactor       = 0.0f,
            .lineWidth                  = 1.0f,
        };
    }

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info(VkSampleCountFlagBits resterizationSamples)
    {
        return
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .rasterizationSamples   = resterizationSamples,
            .sampleShadingEnable    = VK_FALSE,
            .minSampleShading       = 1.0f,
            .pSampleMask            = nullptr,
            .alphaToCoverageEnable  = VK_FALSE,
            .alphaToOneEnable       = VK_FALSE,
        };
    }

    VkPipelineColorBlendStateCreateInfo color_blending_create_info(PointerWithSize<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates)
    {
        return
        {
            .sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .logicOpEnable      = VK_FALSE,
            .logicOp            = VK_LOGIC_OP_COPY, // optional if logicOpEnable == VK_FALSE
            .attachmentCount    = u32(colorBlendAttachmentStates.size),
            .pAttachments       = colorBlendAttachmentStates.ptr,
            .blendConstants     = { 0.0f, 0.0f, 0.0f, 0.0f }, // optional, because color blending is disabled in colorBlendAttachments
        };
    }

    VkPipelineDynamicStateCreateInfo dynamic_state_default_create_info()
    {
        static VkDynamicState dynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        return
        {
            .sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .dynamicStateCount  = array_size(dynamicStates),
            .pDynamicStates     = dynamicStates,
        };
    }

    VkAccessFlags image_layout_to_access_flags(VkImageLayout layout)
    {
        switch (layout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:                         return VkAccessFlags(0);
            case VK_IMAGE_LAYOUT_GENERAL:                           return VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:          return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:  return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:   return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:          return VK_ACCESS_SHADER_READ_BIT;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:              return VK_ACCESS_TRANSFER_READ_BIT;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:              return VK_ACCESS_TRANSFER_WRITE_BIT;
            case VK_IMAGE_LAYOUT_PREINITIALIZED:                    return VK_ACCESS_MEMORY_WRITE_BIT;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                   return VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT;
            default: al_assert_fail("Unsupported VkImageLayout");
        }
        return VkAccessFlags(0);
    }

    VkPipelineStageFlags image_layout_to_pipeline_stage_flags(VkImageLayout layout)
    {
        switch (layout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:                         return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:              return VK_PIPELINE_STAGE_TRANSFER_BIT;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:          return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:          return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:  return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:          return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                   return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // is this correct ?
            default: al_assert_fail("Unsupported VkImageLayout to VkPipelineStageFlags conversion");
        }
        return VkPipelineStageFlags(0);
    }
}
