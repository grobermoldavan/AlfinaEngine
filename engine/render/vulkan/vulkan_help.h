#ifndef AL_VULKAN_HELP_H
#define AL_VULKAN_HELP_H

#include "vulkan_base.h"
#include "engine/types.h"
#include "engine/utilities/utilities.h"
#include "engine/memory/memory.h"

namespace al::vk
{
    const char* VALIDATION_LAYERS[] = 
    {
        "VK_LAYER_KHRONOS_validation",
    };
    // @TODO :  add optional instance extensions ?
    const char* INSTANCE_EXTENSIONS[] = 
    {
        "VK_KHR_surface",
#ifdef _WIN32
        "VK_KHR_win32_surface",
#else
#   error Unsupported platform
#endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    const char* DEVICE_EXTENSIONS[] = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    const uSize MAX_IMAGES_IN_FLIGHT = 3;

    ArrayView<VkLayerProperties> get_available_validation_layers(AllocatorBindings* bindings);
    ArrayView<VkExtensionProperties> get_available_instance_extensions(AllocatorBindings* bindings);

    VkDebugUtilsMessengerCreateInfoEXT get_debug_messenger_create_info(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* userData = nullptr);
    VkDebugUtilsMessengerEXT create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT* createInfo, VkInstance instance, VkAllocationCallbacks* allocationCb = nullptr);

    bool pick_depth_format(VkPhysicalDevice physicalDevice, VkFormat* result);
    ArrayView<VkPhysicalDevice> get_available_physical_devices(VkInstance instance, AllocatorBindings bindings);

    VkCommandBuffer create_command_buffer(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level);

    VkShaderModule create_shader_module(VkDevice device, Tuple<u32*, uSize> bytecode, VkAllocationCallbacks* allocationCb = nullptr);
    void destroy_shader_module(VkDevice device, VkShaderModule module, VkAllocationCallbacks* allocationCb = nullptr);

    bool get_memory_type_index(VkPhysicalDeviceMemoryProperties* props, u32 typeBits, VkMemoryPropertyFlags properties, u32* result);
    void create_buffer(VkDevice device, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* memory);

    uSize advance_render_frame(uSize previousRenderFrame);
}

#endif
