#ifndef AL_VULKAN_HELP_H
#define AL_VULKAN_HELP_H

#include "vulkan_base.h"
#include "engine/types.h"
#include "engine/utilities/utilities.h"
#include "engine/memory/memory.h"

namespace al::utils
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
    const u32 MAX_IMAGES_IN_FLIGHT = 3;

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR    capabilities;
        Array<VkSurfaceFormatKHR>   formats;
        Array<VkPresentModeKHR>     presentModes;
    };

    Array<VkLayerProperties> get_available_validation_layers(AllocatorBindings* bindings);
    Array<VkExtensionProperties> get_available_instance_extensions(AllocatorBindings* bindings);

    VkDebugUtilsMessengerCreateInfoEXT get_debug_messenger_create_info(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* userData = nullptr);
    VkDebugUtilsMessengerEXT create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT* createInfo, VkInstance instance, VkAllocationCallbacks* callbacks = nullptr);
    void destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks* callbacks = nullptr);

    VkCommandPool create_command_pool(VkDevice device, u32 queueFamilyIndex, VkAllocationCallbacks* callbacks, VkCommandPoolCreateFlags flags);
    void destroy_command_pool(VkCommandPool pool, VkDevice device, VkAllocationCallbacks* callbacks);

    SwapChainSupportDetails create_swap_chain_support_details(VkSurfaceKHR surface, VkPhysicalDevice device, AllocatorBindings* bindings);
    void destroy_swap_chain_support_details(SwapChainSupportDetails* details);

    VkSurfaceFormatKHR choose_swap_chain_surface_format(Array<VkSurfaceFormatKHR> available);
    VkPresentModeKHR choose_swap_chain_surface_present_mode(Array<VkPresentModeKHR> available);
    VkExtent2D choose_swap_chain_extent(u32 windowWidth, u32 windowHeight, VkSurfaceCapabilitiesKHR* capabilities);

    Tuple<bool, u32> pick_graphics_queue(Array<VkQueueFamilyProperties> familyProperties);
    Tuple<bool, u32> pick_present_queue(Array<VkQueueFamilyProperties> familyProperties, VkPhysicalDevice device, VkSurfaceKHR surface);
    Tuple<bool, u32> pick_transfer_queue(Array<VkQueueFamilyProperties> familyProperties);

    template<uSize Size>
    Array<VkDeviceQueueCreateInfo> get_queue_create_infos(StaticPointerWithSize<Tuple<bool, u32>, Size> queues, AllocatorBindings* bindings);

    bool pick_depth_stencil_format(VkPhysicalDevice physicalDevice, VkFormat* result);
    Array<VkPhysicalDevice> get_available_physical_devices(VkInstance instance, AllocatorBindings* bindings);
    Array<VkQueueFamilyProperties> get_physical_device_queue_family_properties(VkPhysicalDevice physicalDevice, AllocatorBindings* bindings);
    bool does_physical_device_supports_required_extensions(VkPhysicalDevice device, PointerWithSize<const char* const> extensions, AllocatorBindings* bindings);
    bool does_physical_device_supports_required_features(VkPhysicalDevice device, VkPhysicalDeviceFeatures* requiredFeatures);

    VkImageType pick_image_type(VkExtent3D imageExtent);

    VkCommandBuffer create_command_buffer(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level);

    VkShaderModule create_shader_module(VkDevice device, Tuple<u32*, uSize> bytecode, VkAllocationCallbacks* allocationCb = nullptr);
    void destroy_shader_module(VkDevice device, VkShaderModule module, VkAllocationCallbacks* allocationCb = nullptr);

    bool get_memory_type_index(VkPhysicalDeviceMemoryProperties* props, u32 typeBits, VkMemoryPropertyFlags properties, u32* result);
    void create_buffer(VkDevice device, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* memory);

    u32 advance_render_frame(u32 previousRenderFrame);
}

#endif
