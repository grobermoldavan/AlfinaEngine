#ifndef AL_VULKAN_H
#define AL_VULKAN_H

#include "vulkan_base.h"
#include "vulkan_help.h"
#include "engine/render/renderer_backend_core.h"

#include "engine/types.h"
#include "engine/platform/platform.h"
#include "engine/math/math.h"
#include "engine/utilities/utilities.h"

namespace al
{
    struct VulkanBackend;

    RenderVertex triangle[] = // actually two triangles
    {
        { .position = {  0.0f, -0.5f, 0.1f }, .normal = { }, .uv = { } },
        { .position = {  0.5f,  0.5f, 0.1f }, .normal = { }, .uv = { } },
        { .position = { -0.5f,  0.5f, 0.1f }, .normal = { }, .uv = { } },

        { .position = {  0.2f, -0.3f, 0.2f }, .normal = { }, .uv = { } },
        { .position = {  0.7f,  0.7f, 0.2f }, .normal = { }, .uv = { } },
        { .position = { -0.3f,  0.7f, 0.2f }, .normal = { }, .uv = { } },
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities;
        ArrayView<VkSurfaceFormatKHR>   formats;
        ArrayView<VkPresentModeKHR>     presentModes;
    };

    SwapChainSupportDetails get_swap_chain_support_details(VkSurfaceKHR surface, VkPhysicalDevice device, AllocatorBindings bindings);

    struct VulkanMemoryManager
    {
        static constexpr uSize MAX_CPU_ALLOCATIONS = 4096;
        struct CpuAllocation
        {
            void* ptr;
            uSize size;
        };
        AllocatorBindings       cpuAllocationBindings;
        uSize                   currentNumberOfCpuAllocations;
        CpuAllocation           allocations[MAX_CPU_ALLOCATIONS];
        VkAllocationCallbacks   allocationCallbacks;
    };

    void construct(VulkanMemoryManager* memoryManager, AllocatorBindings bindings);
    void destruct(VulkanMemoryManager* memoryManager);
    AllocatorBindings get_bindings(VulkanMemoryManager* manager);

    struct GPU
    {
        VkPhysicalDevice                    physicalHandle;
        VkDevice                            logicalHandle;
        VkPhysicalDeviceMemoryProperties    memoryProperties;
        // available memory and stuff...
    };

    struct CommandQueues
    {
        static constexpr uSize QUEUES_NUM = 3;
        struct SupportQuery
        {
            VkQueueFamilyProperties* props;
            VkSurfaceKHR surface;
            VkPhysicalDevice device;
            u32 familyIndex;
        };
        struct Queue
        {
            const struct
            {
                bool    (*isSupported)(SupportQuery*);
                bool    isUsedBySwapchain;
            } readOnly;
            VkQueue     handle;
            u32         deviceFamilyIndex;
            bool        isFamilyPresent;
        };
        union
        {
            Queue queues[QUEUES_NUM] =
            {
                {
                    .readOnly =
                    {
                        .isSupported = [](SupportQuery* query) -> bool
                        {
                            return query->props->queueFlags & VK_QUEUE_GRAPHICS_BIT;
                        },
                        .isUsedBySwapchain = true
                    }
                },
                {
                    .readOnly =
                    {
                        .isSupported = [](SupportQuery* query) -> bool
                        {
                            VkBool32 isSupported;
                            vkGetPhysicalDeviceSurfaceSupportKHR(query->device, query->familyIndex, query->surface, &isSupported);
                            return isSupported;
                        },
                        .isUsedBySwapchain = true
                    }
                },
                {
                    .readOnly =
                    {
                        .isSupported = [](SupportQuery* query) -> bool
                        {
                            return query->props->queueFlags & VK_QUEUE_TRANSFER_BIT;
                        },
                        .isUsedBySwapchain = false
                    }
                },
            };
            struct
            {
                Queue graphics;
                Queue present;
                Queue transfer;
            };
        };
    };

    struct CommandPools
    {
        static constexpr uSize POOLS_NUM = 2;
        struct Pool
        {
            VkCommandPool handle;
        };
        union
        {
            // @TODO :  separate pools and buffers for each thread
            // @TODO :  separate pools and buffers for each thread
            // @TODO :  separate pools and buffers for each thread
            // @TODO :  separate pools and buffers for each thread
            Pool pools[POOLS_NUM] =
            {
                { },
                { }
            };
            struct
            {
                Pool graphics;
                Pool transfer;
            };
        };
    };

    struct CommandBuffers
    {
        // @TODO :  separate pools and buffers for each thread
        // @TODO :  separate pools and buffers for each thread
        // @TODO :  separate pools and buffers for each thread
        // @TODO :  separate pools and buffers for each thread
        ArrayView<VkCommandBuffer> primaryBuffers;      // Array size is equal to the number of swap chain images
        ArrayView<VkCommandBuffer> secondaryBuffers;    // Array size is equal to the number of swap chain images
        VkCommandBuffer transferBuffer;
    };

    struct FramebufferAttachment
    {
        VkImage image;
        VkImageView view;
        VkFormat format;
        VkDeviceMemory memory;
    };

    struct Framebuffer
    {
        VkFramebuffer framebuffer;
        VkRenderPass renderPass;
        VkExtent2D extent;
        // ArrayView<FramebufferAttachment> attachments;
    };

    struct SwapChain
    {
        VkSwapchainKHR              handle;
        ArrayView<VkImage>          images;
        ArrayView<VkImageView>      imageViews;
        ArrayView<VkFramebuffer>    framebuffers;
        VkFormat                    format;
        VkExtent2D                  extent;
    };

    // struct RenderStage
    // {
    //     Framebuffer         framebuffer;    // tied to a render pass
    //     VkRenderPass        renderPass;     // tied to nothing
    //     VkPipelineLayout    pipelineLayout; // tied to descriptor sets
    //     VkPipeline          pipeline;       // tied to pepeline layout and render pass (and other stuff like shaders)
    // };

    struct SyncPrimitives
    {
        ArrayView<VkSemaphore>    imageAvailableSemaphores; // Array size is equal to vk::MAX_IMAGES_IN_FLIGHT
        ArrayView<VkSemaphore>    renderFinishedSemaphores; // Array size is equal to vk::MAX_IMAGES_IN_FLIGHT
        ArrayView<VkFence>        inFlightFences;           // Array size is equal to vk::MAX_IMAGES_IN_FLIGHT
        ArrayView<VkFence>        imageInFlightFencesRef;   // Array size is equal to the number of swap chain images
    };

    struct VulkanBackend
    {
        static constexpr PlatformFilePath VERTEX_SHADER_PATH    = { .memory = "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "vert.spv" };
        static constexpr PlatformFilePath FRAGMENT_SHADER_PATH  = { .memory = "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "frag.spv" };

        VulkanMemoryManager         memoryManager;
        VkDebugUtilsMessengerEXT    debugMessenger;
        VkInstance                  instance;
        VkSurfaceKHR                surface;
        GPU                         gpu;
        CommandQueues               queues;
        CommandPools                commandPools;
        CommandBuffers              commandBuffers;
        SwapChain                   swapChain;
        FramebufferAttachment       depthStencil;
        VkRenderPass                simpleRenderPass;
        VkPipelineLayout            pipelineLayout;
        VkPipeline                  pipeline;
        SyncPrimitives              syncPrimitives;

        VkDeviceMemory              vertexBufferMemory;
        VkBuffer                    vertexBuffer;

        // ArrayView<RenderStage>      renderStages;

        uSize                       currentRenderFrame;
        PlatformWindow*             window;
    };

    template<> void renderer_backend_construct      <VulkanBackend> (VulkanBackend* backend, RendererBackendInitData* initData);
    template<> void renderer_backend_render         <VulkanBackend> (VulkanBackend* backend);
    template<> void renderer_backend_destruct       <VulkanBackend> (VulkanBackend* backend);
    template<> void renderer_backend_handle_resize  <VulkanBackend> (VulkanBackend* backend);

    VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);

    Tuple<CommandQueues, VkPhysicalDevice>  pick_physical_device(VulkanBackend* backend);

    void create_instance                    (VulkanBackend* backend, RendererBackendInitData* initData);
    void create_surface                     (VulkanBackend* backend);
    void pick_gpu_and_init_command_queues   (VulkanBackend* backend);
    void create_swap_chain                  (VulkanBackend* backend);
    void create_depth_stencil               (VulkanBackend* backend);
    void create_framebuffers                (VulkanBackend* backend);
    void create_render_passes               (VulkanBackend* backend);
    void create_render_pipelines            (VulkanBackend* backend);
    void create_command_pools               (VulkanBackend* backend);
    void create_command_buffers             (VulkanBackend* backend);
    void create_sync_primitives             (VulkanBackend* backend);
    void create_vertex_buffer               (VulkanBackend* backend);

    void destroy_memory_manager             (VulkanBackend* backend);
    void destroy_instance                   (VulkanBackend* backend);
    void destroy_surface                    (VulkanBackend* backend);
    void destroy_gpu                        (VulkanBackend* backend);
    void destroy_swap_chain                 (VulkanBackend* backend);
    void destroy_depth_stencil              (VulkanBackend* backend);
    void destroy_framebuffers               (VulkanBackend* backend);
    void destroy_render_passes              (VulkanBackend* backend);
    void destroy_render_pipelines           (VulkanBackend* backend);
    void destroy_command_pools              (VulkanBackend* backend);
    void destroy_command_buffers            (VulkanBackend* backend);
    void destroy_sync_primitives            (VulkanBackend* backend);
    void destroy_vertex_buffer              (VulkanBackend* backend);


















#if 0
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
            "VK_LAYER_KHRONOS_validation"
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

        static constexpr PlatformFilePath VERTEX_SHADER_PATH    = { .memory = "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "vert.spv" };
        static constexpr PlatformFilePath FRAGMENT_SHADER_PATH  = { .memory = "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "frag.spv" };

        // @TODO :  unhardcode this value maybe ?
        static constexpr uSize MAX_IMAGES_IN_FLIGHT = 2;

        uSize currentFrameNumber;

        VkAllocationCallbacks           vkAllocationCallbacks;
        VkInstance                      vkInstance;
        VkDebugUtilsMessengerEXT        vkDebugMessenger;
        VkPhysicalDevice                vkPhysicalDevice;
        VkDevice                        vkLogicalDevice;
        VkQueue                         vkGraphicsQueue;
        VkQueue                         vkPresentQueue;
        VkSurfaceKHR                    vkSurface;

        VkSwapchainKHR                  vkSwapChain;
        ArrayView<VkImage>        vkSwapChainImages;
        ArrayView<VkImageView>    vkSwapChainImageViews;
        VkFormat                        vkSwapChainImageFormat;
        VkExtent2D                      vkSwapChainExtent;

        VkRenderPass                    vkRenderPass;
        VkDescriptorSetLayout           vkDescriptorSetLayout;
        VkPipelineLayout                vkPipelineLayout;
        VkPipeline                      vkGraphicsPipeline;
        ArrayView<VkFramebuffer>  vkFramebuffers;

        VkCommandPool                       vkCommandPool;
        ArrayView<VkCommandBuffer>    vkCommandBuffers;

        // @NOTE :  next 3 arrays size is equal to MAX_IMAGES_IN_FLIGHT
        ArrayView<VkSemaphore>    vkImageAvailableSemaphores;
        ArrayView<VkSemaphore>    vkRenderFinishedSemaphores;
        ArrayView<VkFence>        vkInFlightFences;

        // @NOTE :  size is equal to the numbers of swap chain images.
        //          Can contain VK_NULL_HANDLE. If it happends, that means that we don't need to wait for the image fence
        ArrayView<VkFence>        vkImageInFlightFences;

        AllocatorBindings               bindings;
        PlatformWindow*                 window;
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
        FamilyInfo familyInfos[FAMILIES_NUM];
    };

    struct RendererBackendVulkanSwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR            capabilities;
        ArrayView<VkSurfaceFormatKHR> formats;
        ArrayView<VkPresentModeKHR>   presentModes;
    };

    VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);

    template<typename T> uSize                      vulkan_array_view_memory_size                           (ArrayView<T> view);
    template<typename T> void                       construct                                               (ArrayView<T>* view, AllocatorBindings* bindings, uSize size);
    template<typename T> void                       destruct                                                (ArrayView<T> view);

    void                                            vulkan_set_allocation_callbacks                         (RendererBackendVulkan* backend);

    VkDebugUtilsMessengerCreateInfoEXT              vulkan_get_debug_messenger_create_info                  ();
    void                                            vulkan_setup_debug_messenger                            (RendererBackendVulkan* backend);
    void                                            vulkan_destroy_debug_messenger                          (RendererBackendVulkan* backend);

    ArrayView<VkLayerProperties>              vulkan_get_available_validation_layers                  (AllocatorBindings* bindings);
    ArrayView<VkExtensionProperties>          vulkan_get_available_extensions                         (AllocatorBindings* bindings);
    void                                            vulkan_create_vk_instance                               (RendererBackendVulkan* backend, RendererBackendInitData* initData);

    void                                            vulkan_create_surface                                   (RendererBackendVulkan* backend);

    ArrayView<VkQueueFamilyProperties>        vulkan_get_queue_family_properties                      (RendererBackendVulkan* backend, VkPhysicalDevice device);
    bool                                            vulkan_is_queue_families_info_complete                  (VulkanQueueFamiliesInfo* info);
    VulkanQueueFamiliesInfo                         vulkan_get_queue_families_info                          (RendererBackendVulkan* backend, VkPhysicalDevice device);
    ArrayView<VkDeviceQueueCreateInfo>        vulkan_get_queue_create_infos                           (RendererBackendVulkan* backend, VulkanQueueFamiliesInfo* queueFamiliesInfo);

    bool                                            vulkan_does_physical_device_supports_required_extensions(RendererBackendVulkan* backend, VkPhysicalDevice device);
    bool                                            vulkan_is_physical_device_suitable                      (RendererBackendVulkan* backend, VkPhysicalDevice device);
    ArrayView<VkPhysicalDevice>               vulkan_get_available_physical_devices                   (RendererBackendVulkan* backend);
    void                                            vulkan_choose_physical_device                           (RendererBackendVulkan* backend);

    void                                            vulkan_create_logical_device                            (RendererBackendVulkan* backend);

    RendererBackendVulkanSwapChainSupportDetails    vulkan_get_swap_chain_support_details                   (RendererBackendVulkan* backend, VkPhysicalDevice device);
    VkSurfaceFormatKHR                              vulkan_choose_surface_format                            (ArrayView<VkSurfaceFormatKHR> availableFormats);
    VkPresentModeKHR                                vulkan_choose_presentation_mode                         (ArrayView<VkPresentModeKHR> availableModes);
    VkExtent2D                                      vulkan_choose_swap_extent                               (RendererBackendVulkan* backend, VkSurfaceCapabilitiesKHR* surfaceCapabilities);
    void                                            vulkan_create_swap_chain                                (RendererBackendVulkan* backend);
    void                                            vulkan_destroy_swap_chain                               (RendererBackendVulkan* backend);
    void                                            vulkan_create_swap_chain_image_views                    (RendererBackendVulkan* backend);
    void                                            vulkan_destroy_swap_chain_image_views                   (RendererBackendVulkan* backend);

    VkShaderModule                                  vulkan_create_shader_module                             (RendererBackendVulkan* backend, PlatformFile spirvBytecode);
    void                                            vulkan_create_render_pass                               (RendererBackendVulkan* backend);
    void                                            vulkan_create_discriptor_set_layout                     (RendererBackendVulkan* backend);
    void                                            vulkan_create_render_pipeline                           (RendererBackendVulkan* backend);

    void                                            vulkan_create_framebuffers                              (RendererBackendVulkan* backend);
    void                                            vulkan_destroy_framebuffers                             (RendererBackendVulkan* backend);

    void                                            vulkan_create_command_pool                              (RendererBackendVulkan* backend);
    void                                            vulkan_destroy_command_pool                             (RendererBackendVulkan* backend);
    void                                            vulkan_create_command_buffers                           (RendererBackendVulkan* backend);
    void                                            vulkan_destroy_command_buffers                          (RendererBackendVulkan* backend);
    
    void                                            vulkan_create_semaphores                                (RendererBackendVulkan* backend);
    void                                            vulkan_destroy_semaphores                               (RendererBackendVulkan* backend);
    void                                            vulkan_create_fences                                    (RendererBackendVulkan* backend);
    void                                            vulkan_destroy_fences                                   (RendererBackendVulkan* backend);


    void                                            vulkan_cleanup_swap_chain                               (RendererBackendVulkan* backend);
    void                                            vulkan_recreate_swap_chain                              (RendererBackendVulkan* backend);

    template<> void renderer_backend_construct      <RendererBackendVulkan> (RendererBackendVulkan* backend, RendererBackendInitData* initData);
    template<> void renderer_backend_render         <RendererBackendVulkan> (RendererBackendVulkan* backend);
    template<> void renderer_backend_destruct       <RendererBackendVulkan> (RendererBackendVulkan* backend);
    template<> void renderer_backend_handle_resize  <RendererBackendVulkan> (RendererBackendVulkan* backend);
#endif
}

#endif
