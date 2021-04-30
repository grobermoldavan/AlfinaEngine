#ifndef AL_VULKAN_H
#define AL_VULKAN_H

#include "vulkan_base.h"
#include "vulkan_help.h"
#include "engine/render/renderer_backend_core.h"

#include "engine/types.h"
#include "engine/platform/platform.h"
#include "engine/math/math.h"
#include "engine/utilities/utilities.h"

namespace al::vulkan
{
    struct RendererBackend;

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

    struct GpuMemory
    {
        VkDeviceMemory memory;
        VkDeviceSize offsetBytes;
        VkDeviceSize sizeBytes;
    };

    struct VulkanMemoryManager
    {
        static constexpr uSize MAX_CPU_ALLOCATIONS = 4096;
        struct CpuAllocation
        {
            void* ptr;
            uSize size;
        };
        VkAllocationCallbacks   cpu_allocationCallbacks;
        AllocatorBindings       cpu_allocationBindings;
        uSize                   cpu_currentNumberOfAllocations;
        CpuAllocation           cpu_allocations[MAX_CPU_ALLOCATIONS];

        static constexpr uSize GPU_MEMORY_BLOCK_SIZE_BYTES  = 64;
        static constexpr uSize GPU_CHUNK_SIZE_BYTES         = 16 * 1024 * 1024;
        static constexpr uSize GPU_LEDGER_SIZE_BYTES        = GPU_CHUNK_SIZE_BYTES / GPU_MEMORY_BLOCK_SIZE_BYTES / 8;
        static constexpr uSize GPU_MAX_CHUNKS               = 64;
        struct GpuMemoryChunk
        {
            VkDeviceMemory memory;
            u32 memoryTypeIndex;
            u32 usedMemoryBytes;
            u8* ledger;
        };
        struct GpuAllocationRequest
        {
            uSize sizeBytes;
            u32 memoryTypeIndex;
        };
        ArrayView<GpuMemoryChunk>   gpu_chunks;
        ArrayView<u8>               gpu_ledgers;
    };

    void construct(VulkanMemoryManager* memoryManager, AllocatorBindings bindings);
    void destruct(VulkanMemoryManager* memoryManager, VkDevice device);

    GpuMemory gpu_allocate(VulkanMemoryManager* memoryManager, VkDevice device, VulkanMemoryManager::GpuAllocationRequest request);
    void gpu_deallocate(VulkanMemoryManager* memoryManager, VkDevice device, GpuMemory allocation);

    struct MemoryBuffer
    {
        GpuMemory gpuMemory;
        VkBuffer handle;
    };

    MemoryBuffer create_buffer(RendererBackend* backend, VkBufferCreateInfo* createInfo);
    MemoryBuffer create_vertex_buffer(RendererBackend* backend, uSize sizeSytes);
    MemoryBuffer create_staging_buffer(RendererBackend* backend, uSize sizeSytes);
    void destroy_buffer(RendererBackend* backend, MemoryBuffer* buffer);

    void copy_cpu_memory_to_buffer(RendererBackend* backend, MemoryBuffer* buffer, void* data, uSize dataSizeBytes);
    void copy_buffer_to_buffer(RendererBackend* backend, MemoryBuffer* src, MemoryBuffer* dst, uSize sizeBytes);

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

    struct RendererBackend
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

        MemoryBuffer                _vertexBuffer;
        MemoryBuffer                _stagingBuffer;

        // ArrayView<RenderStage>      renderStages;

        uSize                       currentRenderFrame;
        PlatformWindow*             window;
    };

    VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);

    Tuple<CommandQueues, VkPhysicalDevice>  pick_physical_device(RendererBackend* backend);

    void create_instance                    (RendererBackend* backend, RendererBackendInitData* initData);
    void create_surface                     (RendererBackend* backend);
    void pick_gpu_and_init_command_queues   (RendererBackend* backend);
    void create_swap_chain                  (RendererBackend* backend);
    void create_depth_stencil               (RendererBackend* backend);
    void create_framebuffers                (RendererBackend* backend);
    void create_render_passes               (RendererBackend* backend);
    void create_render_pipelines            (RendererBackend* backend);
    void create_command_pools               (RendererBackend* backend);
    void create_command_buffers             (RendererBackend* backend);
    void create_sync_primitives             (RendererBackend* backend);

    void destroy_memory_manager             (RendererBackend* backend);
    void destroy_instance                   (RendererBackend* backend);
    void destroy_surface                    (RendererBackend* backend);
    void destroy_gpu                        (RendererBackend* backend);
    void destroy_swap_chain                 (RendererBackend* backend);
    void destroy_depth_stencil              (RendererBackend* backend);
    void destroy_framebuffers               (RendererBackend* backend);
    void destroy_render_passes              (RendererBackend* backend);
    void destroy_render_pipelines           (RendererBackend* backend);
    void destroy_command_pools              (RendererBackend* backend);
    void destroy_command_buffers            (RendererBackend* backend);
    void destroy_sync_primitives            (RendererBackend* backend);
}

namespace al
{
    template<> void renderer_backend_construct      <struct vulkan::RendererBackend> (vulkan::RendererBackend* backend, RendererBackendInitData* initData);
    template<> void renderer_backend_render         <struct vulkan::RendererBackend> (vulkan::RendererBackend* backend);
    template<> void renderer_backend_destruct       <struct vulkan::RendererBackend> (vulkan::RendererBackend* backend);
    template<> void renderer_backend_handle_resize  <struct vulkan::RendererBackend> (vulkan::RendererBackend* backend);
}

#endif
