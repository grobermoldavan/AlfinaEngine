#ifndef AL_VULKAN_H
#define AL_VULKAN_H

#include "vulkan_base.h"
#include "vulkan_utils.h"
#include "spirv_reflection.h"
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
        { .position = {  0.0f, -0.5f, 0.1f }, .normal = { }, .uv = { 0.0, 0.0 } },
        { .position = {  0.5f,  0.5f, 0.1f }, .normal = { }, .uv = { 1.0, 1.0 } },
        { .position = { -0.5f,  0.5f, 0.1f }, .normal = { }, .uv = { 0.0, 1.0 } },

        { .position = {  0.2f, -0.3f, 0.2f }, .normal = { }, .uv = { 0.0, 1.0 } },
        { .position = {  0.7f,  0.7f, 0.2f }, .normal = { }, .uv = { 1.0, 1.0 } },
        { .position = { -0.3f,  0.7f, 0.2f }, .normal = { }, .uv = { 0.0, 0.0 } },
    };

    u32 triangleIndices[] = { 0, 1, 2, 3, 4, 5 };

#define WHITE   255, 255, 255, 255,
#define RED     255, 0  , 0  , 255,
#define BLUE    0  , 0  , 255, 255,

    u8 testEpicTexture[] =
    {
        RED RED   RED   RED   RED
        RED WHITE WHITE WHITE RED
        RED WHITE BLUE  WHITE RED
        RED WHITE WHITE WHITE RED
        RED RED   RED   RED   RED
    };

#undef RED
#undef WHITE

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
            uSize alignment;
            u32 memoryTypeIndex;
        };
        ArrayView<GpuMemoryChunk>   gpu_chunks;
        ArrayView<u8>               gpu_ledgers;
    };

    void construct_memory_manager(VulkanMemoryManager* memoryManager, AllocatorBindings bindings);
    void destroy_memory_manager(VulkanMemoryManager* memoryManager, VkDevice device);
    uSize memory_chunk_find_aligned_free_space(VulkanMemoryManager::GpuMemoryChunk* chunk, uSize requiredNumberOfBlocks, uSize alignment);
    GpuMemory gpu_allocate(VulkanMemoryManager* memoryManager, VkDevice device, VulkanMemoryManager::GpuAllocationRequest request);
    void gpu_deallocate(VulkanMemoryManager* memoryManager, VkDevice device, GpuMemory allocation);

    struct GPU
    {
        struct CommandQueue
        {
            VkQueue handle;
            u32     deviceFamilyIndex;
            bool    isFamilyPresent;
        };
        union
        {
            CommandQueue commandQueues[3];
            struct
            {
                CommandQueue graphicsQueue;
                CommandQueue presentQueue;
                CommandQueue transferQueue;
            };
        };
        VkPhysicalDevice                    physicalHandle;
        VkDevice                            logicalHandle;
        VkPhysicalDeviceMemoryProperties    memoryProperties;
        VkFormat                            depthStencilFormat;
        // available memory and stuff...
    };

    bool is_command_queues_complete(ArrayView<GPU::CommandQueue> queues);
    void try_pick_graphics_queue(GPU::CommandQueue* queue, ArrayView<VkQueueFamilyProperties> familyProperties);
    void try_pick_present_queue(GPU::CommandQueue* queue, ArrayView<VkQueueFamilyProperties> familyProperties, VkPhysicalDevice device, VkSurfaceKHR surface);
    void try_pick_transfer_queue(GPU::CommandQueue* queue, ArrayView<VkQueueFamilyProperties> familyProperties, u32 graphicsFamilyIndex);

    void fill_required_physical_deivce_features(VkPhysicalDeviceFeatures* features);
    VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface, AllocatorBindings bindings);

    void construct_gpu(GPU* gpu, VkInstance instance, VkSurfaceKHR surface, AllocatorBindings bindings, VkAllocationCallbacks* callbacks);
    void destroy_gpu(GPU* gpu, VkAllocationCallbacks* callbacks);

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities;
        ArrayView<VkSurfaceFormatKHR>   formats;
        ArrayView<VkPresentModeKHR>     presentModes;
    };

    SwapChainSupportDetails create_swap_chain_support_details(VkSurfaceKHR surface, VkPhysicalDevice device, AllocatorBindings bindings);
    void destroy_swap_chain_support_details(SwapChainSupportDetails* details);

    struct SwapChain
    {
        VkSwapchainKHR handle;
        ArrayView<VkImage> images;
        ArrayView<VkImageView> imageViews;
        VkFormat format;
        VkExtent2D extent;
    };

    VkSurfaceFormatKHR choose_swap_chain_surface_format(ArrayView<VkSurfaceFormatKHR> available);
    VkPresentModeKHR choose_swap_chain_surface_present_mode(ArrayView<VkPresentModeKHR> available);
    VkExtent2D choose_swap_chain_extent(u32 windowWidth, u32 windowHeight, VkSurfaceCapabilitiesKHR* capabilities);
    void fill_swap_chain_sharing_mode(VkSharingMode* resultMode, u32* resultCount, ArrayView<u32> resultFamilyIndices, GPU* gpu);
    void construct_swap_chain(SwapChain* swapChain, VkSurfaceKHR surface, GPU* gpu, PlatformWindow* window, AllocatorBindings bindings, VkAllocationCallbacks* callbacks);
    void destroy_swap_chain(SwapChain* swapChain, GPU* gpu, VkAllocationCallbacks* callbacks);

    struct ImageAttachment
    {
        enum Type
        {
            TYPE_1D,
            TYPE_2D,
            TYPE_3D,
        };
        struct ConstructInfo
        {
            VkExtent3D extent;
            VkFormat format;
            VkImageUsageFlags usage;
            VkImageAspectFlags aspect;
            Type type;
        };
        GpuMemory memory;
        VkImage image;
        VkImageView view;
        VkFormat format;
        VkExtent3D extent;
    };

    VkImageType image_attachment_type_to_vk_image_type(ImageAttachment::Type type);
    VkImageViewType image_attachment_type_to_vk_image_view_type(ImageAttachment::Type type);

    void construct_image_attachment(ImageAttachment* attachment, ImageAttachment::ConstructInfo constructInfo, GPU* gpu, VulkanMemoryManager* memoryManager);
    void construct_depth_stencil_attachment(ImageAttachment* attachment, VkExtent2D extent, GPU* gpu, VulkanMemoryManager* memoryManager);
    void construct_color_attachment(ImageAttachment* attachment, VkExtent2D extent, GPU* gpu, VulkanMemoryManager* memoryManager);
    void destroy_image_attachment(ImageAttachment* attachment, GPU* gpu, VulkanMemoryManager* memoryManager);

    void transition_image_layout(GPU* gpu, struct CommandBuffers* buffers, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

    template<typename T>
    struct ThreadLocalStorage
    {
        static constexpr uSize DEFAULT_CAPACITY = 8;
        AllocatorBindings bindings;
        PlatformThreadId* threadIds;
        T* memory;
        uSize size;
        uSize capacity;
    };

    template<typename T> void tls_construct(ThreadLocalStorage<T>* storage, AllocatorBindings bindings);
    template<typename T> void tls_destroy(ThreadLocalStorage<T>* storage);
    template<typename T> T* tls_access(ThreadLocalStorage<T>* storage);

    struct CommandPools
    {
        static constexpr uSize POOLS_NUM = 2;
        struct Pool
        {
            VkCommandPool handle;
        };
        Pool graphics;
        Pool transfer;
    };

    void construct_command_pools(RendererBackend* backend, CommandPools* pools);
    void destroy_command_pools(RendererBackend* backend, CommandPools* pools);
    CommandPools* get_command_pools(RendererBackend* backend);

    struct CommandBuffers
    {
        // @TODO :  if we'll use thread-local storage for command buffers, we probably would want to
        //          move primaryBuffers outside of tls (because they are accessed only from render thread)
        ArrayView<VkCommandBuffer> primaryBuffers;      // Array size is equal to the number of swap chain images
        ArrayView<VkCommandBuffer> secondaryBuffers;    // Array size is equal to the number of swap chain images
        VkCommandBuffer transferBuffer;
    };

    void construct_command_buffers(RendererBackend* backend, CommandBuffers* buffers);
    void destroy_command_buffers(RendererBackend* backend, CommandBuffers* buffers);

    struct MemoryBuffer
    {
        GpuMemory gpuMemory;
        VkBuffer handle;
    };

    MemoryBuffer create_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, VkBufferCreateInfo* createInfo, VkMemoryPropertyFlags memoryProperty);
    MemoryBuffer create_vertex_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes);
    MemoryBuffer create_index_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes);
    MemoryBuffer create_staging_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes);
    MemoryBuffer create_uniform_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes);
    void destroy_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, MemoryBuffer* buffer);

    void copy_cpu_memory_to_buffer(VkDevice device, void* data, MemoryBuffer* buffer, uSize dataSizeBytes);
    void copy_buffer_to_buffer(GPU* gpu, CommandBuffers* buffers, MemoryBuffer* src, MemoryBuffer* dst, uSize sizeBytes, uSize srcOffsetBytes = 0, uSize dstOffsetBytes = 0);
    void copy_buffer_to_image(GPU* gpu, CommandBuffers* buffers, MemoryBuffer* src, VkImage dst, VkExtent3D extent);

    struct HardcodedDescriptorsSets
    {
        struct Ubo
        {
            f32_3 color;
        };
        struct BindingDesc
        {
            VkShaderStageFlags stageFlags;
            VkDescriptorType type;
        };
        static constexpr BindingDesc BINDING_DESCS[1] =
        {
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,   // Can be retrieved from a reflection data
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,  // Can be retrieved from a reflection data
            },
        };
        VkDescriptorSetLayout layout;
        VkDescriptorPool pool;
        ArrayView<VkDescriptorSet> sets;    // Array size is equal to the number of swap chain images
        ArrayView<MemoryBuffer> buffers;    // Array size is equal to the number of swap chain images
    };

    void create_test_descriptor_sets(RendererBackend* backend, HardcodedDescriptorsSets* sets);
    void destroy_test_descriptor_sets(RendererBackend* backend, HardcodedDescriptorsSets* sets);

    struct SyncPrimitives
    {
        ArrayView<VkSemaphore>    imageAvailableSemaphores; // Array size is equal to vk::MAX_IMAGES_IN_FLIGHT
        ArrayView<VkSemaphore>    renderFinishedSemaphores; // Array size is equal to vk::MAX_IMAGES_IN_FLIGHT
        ArrayView<VkFence>        inFlightFences;           // Array size is equal to vk::MAX_IMAGES_IN_FLIGHT
        ArrayView<VkFence>        imageInFlightFencesRef;   // Array size is equal to the number of swap chain images
    };

    struct RendererBackend
    {
        static constexpr PlatformFilePath VERTEX_SHADER_PATH    = { .memory = "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "triangle.vert.spv" };
        static constexpr PlatformFilePath FRAGMENT_SHADER_PATH  = { .memory = "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "triangle.frag.spv" };

        VulkanMemoryManager         memoryManager;
        VkDebugUtilsMessengerEXT    debugMessenger;
        VkInstance                  instance;
        VkSurfaceKHR                surface;
        GPU                         gpu;
        SwapChain                   swapChain;
        ImageAttachment             depthStencil;

        VkRenderPass                renderPass;
        ArrayView<VkFramebuffer>    passFramebuffers;
        HardcodedDescriptorsSets    descriptorSets;
        VkPipelineLayout            pipelineLayout;
        VkPipeline                  pipeline;

        SyncPrimitives              syncPrimitives;

        CommandPools                commandPools;
        CommandBuffers              commandBuffers;

        MemoryBuffer                _vertexBuffer;
        MemoryBuffer                _indexBuffer;
        MemoryBuffer                _stagingBuffer;
        ImageAttachment             _texture;
        VkSampler                   _textureSampler;

        uSize                       currentRenderFrame;
        PlatformWindow*             window;
    };

    VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);

    void construct_instance         (VkInstance* instance, VkDebugUtilsMessengerEXT* debugMessenger, RendererBackendInitData* initData, AllocatorBindings bindings, VkAllocationCallbacks* callbacks);
    void construct_surface          (VkSurfaceKHR* surface, PlatformWindow* window, VkInstance instance, VkAllocationCallbacks* allocationCallbacks);
    void construct_render_pass      (VkRenderPass* pass, SwapChain* swapChain, ImageAttachment* depthStencil, GPU* gpu, VulkanMemoryManager* memoryManager);
    void construct_framebuffers     (ArrayView<VkFramebuffer>* framebuffers, VkRenderPass pass, SwapChain* swapChain, VulkanMemoryManager* memoryManager, GPU* gpu, ImageAttachment* depthStencil);
    void construct_render_pipeline  (RendererBackend* backend);
    void create_sync_primitives     (RendererBackend* backend);

    void destroy_instance           (VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks* callbacks);
    void destroy_surface            (VkSurfaceKHR surface, VkInstance instance, VkAllocationCallbacks* callbacks);
    void destroy_render_pass        (VkRenderPass pass, GPU* gpu, VulkanMemoryManager* memoryManager);
    void destroy_framebuffers       (ArrayView<VkFramebuffer> framebuffers, GPU* gpu, VulkanMemoryManager* memoryManager);
    void destroy_render_pipeline    (RendererBackend* backend);
    void destroy_sync_primitives    (RendererBackend* backend);
}

namespace al
{
    template<> void renderer_backend_construct      <struct vulkan::RendererBackend> (vulkan::RendererBackend* backend, RendererBackendInitData* initData);
    template<> void renderer_backend_render         <struct vulkan::RendererBackend> (vulkan::RendererBackend* backend);
    template<> void renderer_backend_destruct       <struct vulkan::RendererBackend> (vulkan::RendererBackend* backend);
    template<> void renderer_backend_handle_resize  <struct vulkan::RendererBackend> (vulkan::RendererBackend* backend);
}

#endif
