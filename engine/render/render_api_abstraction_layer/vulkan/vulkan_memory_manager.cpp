
#include "vulkan_memory_manager.h"

namespace al
{
    void vulkan_memory_manager_construct(VulkanMemoryManager* memoryManager, VulkanMemoryManagerCreateInfo* createInfo)
    {
        memoryManager->cpu_persistentAllocator = *createInfo->persistentAllocator;
        memoryManager->cpu_frameAllocator = *createInfo->frameAllocator;
        memoryManager->cpu_allocationCallbacks =
        {
            .pUserData = memoryManager,
            .pfnAllocation = 
                [](void* pUserData, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    VulkanMemoryManager* manager = static_cast<VulkanMemoryManager*>(pUserData);
                    void* result = allocate(&manager->cpu_persistentAllocator, size);
                    al_vk_assert(manager->cpu_currentNumberOfAllocations < VulkanMemoryManager::MAX_CPU_ALLOCATIONS);
                    manager->cpu_allocations[manager->cpu_currentNumberOfAllocations++] =
                    {
                        .ptr = result,
                        .size = size,
                    };
                    return result;
                },
            .pfnReallocation =
                [](void* pUserData, void* pOriginal, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    VulkanMemoryManager* manager = static_cast<VulkanMemoryManager*>(pUserData);
                    void* result = nullptr;
                    for (uSize it = 0; it < manager->cpu_currentNumberOfAllocations; it++)
                    {
                        if (manager->cpu_allocations[it].ptr == pOriginal)
                        {
                            deallocate(&manager->cpu_persistentAllocator, manager->cpu_allocations[it].ptr, manager->cpu_allocations[it].size);
                            result = allocate(&manager->cpu_persistentAllocator, size);
                            manager->cpu_allocations[it] =
                            {
                                .ptr = result,
                                .size = size,
                            };
                            break;
                        }
                    }
                    return result;
                },
            .pfnFree =
                [](void* pUserData, void* pMemory)
                {
                    VulkanMemoryManager* manager = static_cast<VulkanMemoryManager*>(pUserData);
                    for (uSize it = 0; it < manager->cpu_currentNumberOfAllocations; it++)
                    {
                        if (manager->cpu_allocations[it].ptr == pMemory)
                        {
                            deallocate(&manager->cpu_persistentAllocator, manager->cpu_allocations[it].ptr, manager->cpu_allocations[it].size);
                            manager->cpu_allocations[it] = manager->cpu_allocations[manager->cpu_currentNumberOfAllocations - 1];
                            manager->cpu_currentNumberOfAllocations -= 1;
                            break;
                        }
                    }
                },
            .pfnInternalAllocation  = nullptr,
            .pfnInternalFree        = nullptr
        };
        array_construct(&memoryManager->gpu_chunks, &memoryManager->cpu_persistentAllocator, VulkanMemoryManager::GPU_MAX_CHUNKS);
        array_construct(&memoryManager->gpu_ledgers, &memoryManager->cpu_persistentAllocator, VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES * VulkanMemoryManager::GPU_MAX_CHUNKS);
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            chunk->ledger = &memoryManager->gpu_ledgers[it * VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES];
        }
    }

    void vulkan_memory_manager_destroy(VulkanMemoryManager* memoryManager)
    {
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (chunk->memory)
            {
                vkFreeMemory(memoryManager->device, chunk->memory, &memoryManager->cpu_allocationCallbacks);
            }
        }
        array_destruct(&memoryManager->gpu_chunks);
        array_destruct(&memoryManager->gpu_ledgers);
    }

    void vulkan_memory_manager_set_device(VulkanMemoryManager* memoryManager, VkDevice device)
    {
        memoryManager->device = device;
    }

    bool gpu_is_valid_memory(VulkanMemoryManager::Memory memory)
    {
        return memory.memory != VK_NULL_HANDLE;
    }

    VulkanMemoryManager::Memory gpu_allocate(VulkanMemoryManager* memoryManager, VulkanMemoryManager::GpuAllocationRequest request)
    {
        auto setInUse = [](VulkanMemoryManager::GpuMemoryChunk* chunk, uSize numBlocks, uSize startBlock)
        {
            for (uSize it = 0; it < numBlocks; it++)
            {
                uSize currentByte = (startBlock + it) / 8;
                uSize currentBit = (startBlock + it) % 8;
                u8* byte = &chunk->ledger[currentByte];
                *byte |= (1 << currentBit);
            }
            chunk->usedMemoryBytes += numBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            al_vk_assert(chunk->usedMemoryBytes <= VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        };
        al_vk_assert(request.sizeBytes <= VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        uSize requiredNumberOfBlocks = 1 + ((request.sizeBytes - 1) / VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES);
        // 1. Try to find memory in available chunks
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (!chunk->memory)
            {
                continue;
            }
            if (chunk->memoryTypeIndex != request.memoryTypeIndex)
            {
                continue;
            }
            if ((VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES - chunk->usedMemoryBytes) < request.sizeBytes)
            {
                continue;
            }
            uSize inChunkOffset = memory_chunk_find_aligned_free_space(chunk, requiredNumberOfBlocks, request.alignment);
            if (inChunkOffset == VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES)
            {
                continue;
            }
            setInUse(chunk, requiredNumberOfBlocks, inChunkOffset);
            return
            {
                .memory = chunk->memory,
                .offsetBytes = inChunkOffset * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
                .sizeBytes = requiredNumberOfBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
            };
        }
        // 2. Try to allocate new chunk
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (chunk->memory)
            {
                continue;
            }
            // Allocating new chunk
            {
                VkMemoryAllocateInfo memoryAllocateInfo = { };
                memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                memoryAllocateInfo.allocationSize = VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES;
                memoryAllocateInfo.memoryTypeIndex = request.memoryTypeIndex;
                al_vk_check(vkAllocateMemory(memoryManager->device, &memoryAllocateInfo, &memoryManager->cpu_allocationCallbacks, &chunk->memory));
                chunk->memoryTypeIndex = request.memoryTypeIndex;
                std::memset(chunk->ledger, 0, VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES);
            }
            // Allocating memory in new chunk
            uSize inChunkOffset = memory_chunk_find_aligned_free_space(chunk, requiredNumberOfBlocks, request.alignment);
            al_vk_assert(inChunkOffset != VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES);
            setInUse(chunk, requiredNumberOfBlocks, inChunkOffset);
            return
            {
                .memory = chunk->memory,
                .offsetBytes = inChunkOffset * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
                .sizeBytes = requiredNumberOfBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
            };
        }
        al_vk_assert_fail("Out of memory");
        return { };
    }

    void gpu_deallocate(VulkanMemoryManager* memoryManager, VulkanMemoryManager::Memory allocation)
    {
        auto setFree = [](VulkanMemoryManager::GpuMemoryChunk* chunk, uSize numBlocks, uSize startBlock)
        {
            for (uSize it = 0; it < numBlocks; it++)
            {
                uSize currentByte = (startBlock + it) / 8;
                uSize currentBit = (startBlock + it) % 8;
                u8* byte = &chunk->ledger[currentByte];
                *byte &= ~(1 << currentBit);
            }
            chunk->usedMemoryBytes -= numBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            al_vk_assert(chunk->usedMemoryBytes <= VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        };
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (chunk->memory != allocation.memory)
            {
                continue;
            }
            uSize startBlock = allocation.offsetBytes / VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            uSize numBlocks = allocation.sizeBytes / VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            setFree(chunk, numBlocks, startBlock);
            if (chunk->usedMemoryBytes == 0)
            {
                // Chunk is empty, so we free it
                vkFreeMemory(memoryManager->device, chunk->memory, &memoryManager->cpu_allocationCallbacks);
                chunk->memory = VK_NULL_HANDLE;
            }
            break;
        }
    }

    uSize memory_chunk_find_aligned_free_space(VulkanMemoryManager::GpuMemoryChunk* chunk, uSize requiredNumberOfBlocks, uSize alignment)
    {
        uSize freeCount = 0;
        uSize startBlock = 0;
        uSize currentBlock = 0;
        for (uSize ledgerIt = 0; ledgerIt < VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES; ledgerIt++)
        {
            u8 byte = chunk->ledger[ledgerIt];
            if (byte == 255)
            {
                freeCount = 0;
                currentBlock += 8;
                continue;
            }
            for (uSize byteIt = 0; byteIt < 8; byteIt++)
            {
                if (byte & (1 << byteIt))
                {
                    freeCount = 0;
                }
                else
                {
                    if (freeCount == 0)
                    {
                        uSize currentBlockOffsetBytes = (ledgerIt * 8 + byteIt) * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
                        bool isAlignedCorrectly = (currentBlockOffsetBytes % alignment) == 0;
                        if (isAlignedCorrectly)
                        {
                            startBlock = currentBlock;
                            freeCount += 1;
                        }
                    }
                    else
                    {
                        freeCount += 1;
                    }
                }
                currentBlock += 1;
                if (freeCount == requiredNumberOfBlocks)
                {
                    goto block_found;
                }
            }
        }
        startBlock = VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES;
        block_found:
        return startBlock;
    }
}
