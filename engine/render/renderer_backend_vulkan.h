#ifndef AL_VULKAN_H
#define AL_VULKAN_H

// @TODO :  make renderer platform-agnostic and remove all platform-specific things to platform folder
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "engine/types.h"
#include "engine/platform/platform.h"
#include "engine/render/renderer_backend_core.h"

namespace al
{
    template<typename T>
    struct RendererBackendVulkanArrayView
    {
        T* memory;
        u32 count;
    };

    struct RendererBackendVulkan
    {
        static constexpr const char* ALLOCATION_SCOPE_TO_STR[] = 
        {
            "VK_SYSTEM_ALLOCATION_SCOPE_COMMAND",
            "VK_SYSTEM_ALLOCATION_SCOPE_OBJECT",
            "VK_SYSTEM_ALLOCATION_SCOPE_CACHE",
            "VK_SYSTEM_ALLOCATION_SCOPE_DEVICE",
            "VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE",
        };
        static constexpr const char* REQUIRED_VALIDATION_LAYERS[] = 
        {
            "VK_LAYER_KHRONOS_validation",
        };
        static constexpr const char* REQUIRED_EXTENSIONS[] = 
        {   // @TODO :  make backend platform-agnostic and remove all platform-specific things to platform folder
            "VK_KHR_surface",                   // To connect vulkan backend to window
            "VK_KHR_win32_surface",             // Platform specific extension for interacting with window system
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,  // To setup debug callback function
        };
        static constexpr const char* REQUIRED_DEVICE_EXTENSIONS[] = 
        {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        static constexpr uSize REQUIRED_VALIDATION_LAYERS_COUNT = sizeof(REQUIRED_VALIDATION_LAYERS) / sizeof(REQUIRED_VALIDATION_LAYERS[0]);
        static constexpr uSize REQUIRED_EXTENSIONS_COUNT        = sizeof(REQUIRED_EXTENSIONS)        / sizeof(REQUIRED_EXTENSIONS[0]);
        static constexpr uSize REQUIRED_DEVICE_EXTENSIONS_COUNT = sizeof(REQUIRED_DEVICE_EXTENSIONS) / sizeof(REQUIRED_DEVICE_EXTENSIONS[0]);

        static constexpr PlatformFilePath VERTEX_SHADER_PATH    = { .memory = "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "vert.spv"   };
        static constexpr PlatformFilePath FRAGMENT_SHADER_PATH  = { .memory = "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "frag.spv" };

        // @TODO :  unhardcode this value maybe ?
        static constexpr uSize MAX_IMAGES_IN_FLIGHT = 2;

        uSize currentFrameNumber;

        VkAllocationCallbacks                           vkAllocationCallbacks;
        VkInstance                                      vkInstance;
        VkDebugUtilsMessengerEXT                        vkDebugMessenger;
        VkPhysicalDevice                                vkPhysicalDevice;
        VkDevice                                        vkLogicalDevice;
        VkQueue                                         vkGraphicsQueue;
        VkQueue                                         vkPresentQueue;
        VkSurfaceKHR                                    vkSurface;

        VkSwapchainKHR                                  vkSwapChain;
        RendererBackendVulkanArrayView<VkImage>         vkSwapChainImages;
        RendererBackendVulkanArrayView<VkImageView>     vkSwapChainImageViews;
        VkFormat                                        vkSwapChainImageFormat;
        VkExtent2D                                      vkSwapChainExtent;

        VkRenderPass                                    vkRenderPass;
        VkPipelineLayout                                vkPipelineLayout;
        VkPipeline                                      vkGraphicsPipeline;
        RendererBackendVulkanArrayView<VkFramebuffer>   vkFramebuffers;

        VkCommandPool                                   vkCommandPool;
        RendererBackendVulkanArrayView<VkCommandBuffer> vkCommandBuffers;

        // @NOTE :  next 3 arrays size is equal to MAX_IMAGES_IN_FLIGHT
        RendererBackendVulkanArrayView<VkSemaphore>     vkImageAvailableSemaphores;
        RendererBackendVulkanArrayView<VkSemaphore>     vkRenderFinishedSemaphores;
        RendererBackendVulkanArrayView<VkFence>         vkInFlightFences;

        // @NOTE :  size is equal to the numbers of swap chain images.
        //          Can contain VK_NULL_HANDLE. If it happends, that means that we don't need to wait for the image fence
        RendererBackendVulkanArrayView<VkFence>         vkImageInFlightFences;

        AllocatorBindings           bindings;
        PlatformWindow*             window;
    };

    struct VulkanQueueFamiliesInfo
    {
        struct FamilyInfo
        {
            u32 index;
            bool isPresent;
        };
        static constexpr uSize FAMILIES_NUM             = 2;
        static constexpr uSize GRAPHICS_FAMILY          = 0;    // queue family that supports drawing commands
        static constexpr uSize SURFACE_PRESENT_FAMILY   = 1;    // queue family capable of presenting images to surface
        FamilyInfo familyInfoIndex[FAMILIES_NUM];
    };

    struct RendererBackendVulkanSwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR                            capabilities;
        RendererBackendVulkanArrayView<VkSurfaceFormatKHR>  formats;
        RendererBackendVulkanArrayView<VkPresentModeKHR>    presentModes;
    };

    template<typename T>
    uSize vulkan_array_view_memory_size(RendererBackendVulkanArrayView<T>* view);

    void renderer_backend_construct (RendererBackendVulkan* backend, RendererBackendInitData* initData);
    void renderer_backend_destruct  (RendererBackendVulkan* backend);
    void renderer_backend_render    (RendererBackendVulkan* backend);

    VKAPI_ATTR VkBool32 VKAPI_CALL
    vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);

    void                                vulkan_set_allocation_callbacks        (RendererBackendVulkan* backend);
    void                                vulkan_create_vk_instance              (RendererBackendVulkan* backend, RendererBackendInitData* initData);
    VkDebugUtilsMessengerCreateInfoEXT  vulkan_get_debug_messenger_create_info ();
    void                                vulkan_get_setup_debug_messenger       (RendererBackendVulkan* backend);
    void                                vulkan_destroy_debug_messenger         (RendererBackendVulkan* backend);
    
    RendererBackendVulkanSwapChainSupportDetails vulkan_get_swap_chain_support_details(RendererBackendVulkan* backend, VkPhysicalDevice device);
}

#endif
