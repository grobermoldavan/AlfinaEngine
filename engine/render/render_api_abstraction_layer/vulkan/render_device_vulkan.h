#ifndef AL_DEVICE_VULKAN_H
#define AL_DEVICE_VULKAN_H

#include "../base/render_device.h"
#include "../base/enums.h"
#include "vulkan_base.h"

namespace al
{
    struct Texture;
    struct TextureVulkan;

    struct VulkanMemoryManagerCreateInfo
    {
        AllocatorBindings* persistentAllocator;
        AllocatorBindings* frameAllocator;
    };

    struct VulkanMemoryManager
    {
        //
        // CPU-side
        //
        static constexpr uSize MAX_CPU_ALLOCATIONS = 4096;
        struct CpuAllocation
        {
            void* ptr;
            uSize size;
        };
        AllocatorBindings cpu_persistentAllocator;
        AllocatorBindings cpu_frameAllocator;
        VkAllocationCallbacks cpu_allocationCallbacks;
        uSize cpu_currentNumberOfAllocations;
        CpuAllocation cpu_allocations[MAX_CPU_ALLOCATIONS];
        //
        // GPU-side
        //
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
        struct Memory
        {
            VkDeviceMemory memory;
            VkDeviceSize offsetBytes;
            VkDeviceSize sizeBytes;
        };
        Array<GpuMemoryChunk> gpu_chunks;
        Array<u8> gpu_ledgers;
        VkDevice device;
    };

    void vulkan_memory_manager_construct(VulkanMemoryManager* memoryManager, VulkanMemoryManagerCreateInfo* createInfo);
    void vulkan_memory_manager_destroy(VulkanMemoryManager* memoryManager);
    void vulkan_memory_manager_set_device(VulkanMemoryManager* memoryManager, VkDevice device);
    bool gpu_is_valid_memory(VulkanMemoryManager::Memory memory);
    VulkanMemoryManager::Memory gpu_allocate(VulkanMemoryManager* memoryManager, VulkanMemoryManager::GpuAllocationRequest request);
    void gpu_deallocate(VulkanMemoryManager* memoryManager, VulkanMemoryManager::Memory allocation);
    uSize memory_chunk_find_aligned_free_space(VulkanMemoryManager::GpuMemoryChunk* chunk, uSize requiredNumberOfBlocks, uSize alignment);

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
        };
        StaticPointerWithSize<CommandQueue, MAX_UNIQUE_COMMAND_QUEUES> commandQueues;
        VkPhysicalDevice physicalHandle;
        VkDevice logicalHandle;
        VkPhysicalDeviceMemoryProperties memoryProperties;
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

    struct RenderDeviceVulkan : RenderDevice
    {
        VulkanMemoryManager memoryManager;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VulkanGpu gpu;
        VulkanSwapChain swapChain;
    };

    RenderDevice* vulkan_device_create(RenderDeviceCreateInfo* createInfo);
    void vulkan_device_destroy(RenderDevice* device);
    uSize vulkan_get_swap_chain_textures_num(RenderDevice* device);
    Texture* vulkan_get_swap_chain_texture(RenderDevice* device, uSize index);
}

#endif
