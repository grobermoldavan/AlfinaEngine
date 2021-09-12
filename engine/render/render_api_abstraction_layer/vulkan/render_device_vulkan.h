#ifndef AL_DEVICE_VULKAN_H
#define AL_DEVICE_VULKAN_H

#include "../base/render_device.h"
#include "../base/enums.h"
#include "vulkan_base.h"
#include "vulkan_memory_manager.h"

namespace al
{
    struct Texture;
    struct TextureVulkan;
    struct CommandBufferVulkan;

    struct VulkanInstanceCreateInfo
    {
        AllocatorBindings* persistentAllocator;
        AllocatorBindings* frameAllocator;
        VkAllocationCallbacks* callbacks;
        const char* applicationName;
        bool isDebug;
    };

    struct VulkanInstanceCreateResult
    {
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
    };

    VulkanInstanceCreateResult vulkan_instance_construct(VulkanInstanceCreateInfo* createInfo);
    void vulkan_instance_destroy(VkInstance instance, VkAllocationCallbacks* callbacks, VkDebugUtilsMessengerEXT messenger);

    struct VulkanSurfaceCreateInfo
    {
        PlatformWindow* window;
        VkInstance instance;
        VkAllocationCallbacks* callbacks;
    };

    VkSurfaceKHR vulkan_surface_create(VulkanSurfaceCreateInfo* createInfo);
    void vulkan_surface_destroy(VkSurfaceKHR surface, VkInstance instance, VkAllocationCallbacks* callbacks);

    struct VulkanGpu
    {
        static constexpr uSize MAX_UNIQUE_COMMAND_QUEUES = 3;
        using CommandQueueFlags = u32;
        using GpuFlags = u64;
        enum Flags : GpuFlags
        {
            HAS_STENCIL = GpuFlags(1) << 0,
        };
        struct CommandQueue
        {
            enum Flags : CommandQueueFlags
            {
                GRAPHICS    = CommandQueueFlags(1) << 0,
                PRESENT     = CommandQueueFlags(1) << 1,
                TRANSFER    = CommandQueueFlags(1) << 2,
            };
            VkQueue handle;
            u32 queueFamilyIndex;
            CommandQueueFlags flags;
            VkCommandPool commandPoolHandle;
        };
        VkPhysicalDeviceProperties deviceProperties_10;
        VkPhysicalDeviceVulkan11Properties deviceProperties_11;
        VkPhysicalDeviceVulkan12Properties deviceProperties_12;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        CommandQueue commandQueues[MAX_UNIQUE_COMMAND_QUEUES];
        VkPhysicalDevice physicalHandle;
        VkDevice logicalHandle;
        VkFormat depthStencilFormat;
        GpuFlags flags;
    };

    struct VulkanGpuCreateInfo
    {
        VkInstance instance;
        VkSurfaceKHR surface;
        AllocatorBindings* persistentAllocator;
        AllocatorBindings* frameAllocator;
        VkAllocationCallbacks* callbacks;
    };

    VulkanGpu::CommandQueue* get_command_queue(VulkanGpu* gpu, VulkanGpu::CommandQueueFlags flags);
    void fill_required_physical_deivce_features(VkPhysicalDeviceFeatures* features);
    VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface, AllocatorBindings* bindings);
    void vulkan_gpu_construct(VulkanGpu* gpu, VulkanGpuCreateInfo* createInfo);
    void vulkan_gpu_destroy(VulkanGpu* gpu, VkAllocationCallbacks* callbacks);
    VkSampleCountFlags vulkan_gpu_get_supported_framebuffer_multisample_types(VulkanGpu* gpu);
    VkSampleCountFlags vulkan_gpu_get_supported_image_multisample_types(VulkanGpu* gpu);

    struct VulkanSwapChain
    {
        struct Image
        {
            VkImage handle;
            VkImageView view;
        };
        VkSwapchainKHR handle;
        VkSurfaceFormatKHR surfaceFormat;
        VkExtent2D extent;
        Array<Image> images;
        Array<TextureVulkan> textures;
    };

    struct VulkanSwapChainCreateInfo
    {
        VulkanGpu* gpu;
        VkSurfaceKHR surface;
        AllocatorBindings* persistentAllocator;
        AllocatorBindings* frameAllocator;
        VkAllocationCallbacks* callbacks;
        u32 windowWidth;
        u32 windowHeight;
    };

    void fill_swap_chain_sharing_mode(VkSharingMode* resultMode, u32* resultCount, PointerWithSize<u32> resultFamilyIndices, VulkanGpu* gpu);
    void vulkan_swap_chain_construct(VulkanSwapChain* swapChain, VulkanSwapChainCreateInfo* createInfo);
    void vulkan_swap_chain_destroy(VulkanSwapChain* swapChain, VkDevice device, VkAllocationCallbacks* callbacks);

    /*
        inFlightData -                      contains data specific to each image in flight
        swapChainImageToInFlightFrameMap -  shows what swap chain image is used by what inFlightData
        activeFrameInFlightIndex -          index of active image in flight
        activeSwapChainImageIndex -         index of active swap chain image
        commandBuffers -                    all command buffers submitted to this image in flight
        imageAvailableSemaphore -           fires when swap chain image, used by a given inFlightData,
                                            becomes available
    */
    struct VulkanInFlightData
    {
        struct PerImageInFlightData
        {
            DynamicArray<CommandBufferVulkan*> commandBuffers;
            VkSemaphore imageAvailableSemaphore;
        };
        static constexpr u32 UNUSED_IN_FLIGHT_DATA_REF = ~u32(0);
        static constexpr u32 NUM_IMAGES_IN_FLIGHT = 3;
        PerImageInFlightData inFlightData[NUM_IMAGES_IN_FLIGHT];
        Array<u32> swapChainImageToInFlightFrameMap;
        u32 activeFrameInFlightIndex;
        u32 activeSwapChainImageIndex;
    };

    struct VulkanInFlightDataCreateInfo
    {
        VulkanGpu* gpu;
        VulkanSwapChain* swapChain;
        VkAllocationCallbacks* allocationCallbacks;
        AllocatorBindings* persistenAllocator;
    };

    void vulkan_in_flight_data_construct(VulkanInFlightData* info, VulkanInFlightDataCreateInfo* createInfo);
    void vulkan_in_flight_data_destroy(VulkanInFlightData* info, VkDevice device, VkAllocationCallbacks* callbacks);
    void vulkan_in_flight_data_advance_frame(VulkanInFlightData* data, VkDevice device, VulkanSwapChain* swapChain);
    VulkanInFlightData::PerImageInFlightData* vulkan_in_flight_data_get_current(VulkanInFlightData* data);

    struct RenderDeviceVulkan : RenderDevice
    {
        using FlagsT = u64;
        struct Flags { enum : FlagsT
        {
            IS_IN_RENDER_FRAME      = FlagsT(1) << 0,
            HAS_SUBMITTED_BUFFERS   = FlagsT(1) << 1,
        }; };
        VulkanMemoryManager memoryManager;
        VulkanInFlightData inFlightData;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VulkanGpu gpu;
        VulkanSwapChain swapChain;
        FlagsT flags;
        CommandBufferVulkan* lastSubmittedCommandBuffer;
    };

    RenderDevice* vulkan_device_create(RenderDeviceCreateInfo* createInfo);
    void vulkan_device_destroy(RenderDevice* device);
    void vulkan_device_wait(RenderDevice* device);
    uSize vulkan_get_swap_chain_textures_num(RenderDevice* device);
    Texture* vulkan_get_swap_chain_texture(RenderDevice* device, uSize index);
    uSize vulkan_get_active_swap_chain_texture_index(RenderDevice* device);
    void vulkan_begin_frame(RenderDevice* device);
    void vulkan_end_frame(RenderDevice* device);

    void vulkan_test(RenderDevice* device, RenderPipeline* pipeline);
}

#endif
