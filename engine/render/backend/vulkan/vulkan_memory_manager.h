#ifndef AL_VULKAN_MEMORY_MANAGER_H
#define AL_VULKAN_MEMORY_MANAGER_H

#include "engine/types.h"
#include "engine/memory/memory.h"
#include "engine/utilities/utilities.h"
#include "vulkan_base.h"

namespace al
{
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
        VkAllocationCallbacks cpu_allocationCallbacks;
        AllocatorBindings cpu_allocationBindings;
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
    };

    void construct_memory_manager(VulkanMemoryManager* memoryManager, AllocatorBindings* bindings);
    void destroy_memory_manager(VulkanMemoryManager* memoryManager, VkDevice device);
    bool gpu_is_valid_memory(VulkanMemoryManager::Memory memory);
    VulkanMemoryManager::Memory gpu_allocate(VulkanMemoryManager* memoryManager, VkDevice device, VulkanMemoryManager::GpuAllocationRequest request);
    void gpu_deallocate(VulkanMemoryManager* memoryManager, VkDevice device, VulkanMemoryManager::Memory allocation);
}

#endif
