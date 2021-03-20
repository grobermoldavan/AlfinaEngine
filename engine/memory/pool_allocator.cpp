
#include <algorithm>    // for std::sort
#include <cstring>      // for std::memset, std::memcpy

#include "pool_allocator.h"

#include "utilities/memory.h"
#include "utilities/procedural_wrap.h"

namespace al::engine
{
    void construct(MemoryBucket* bucket)
    {
        wrap_construct(&bucket->memoryMutex);
        bucket->blockSizeBytes  = 0;
        bucket->blockCount      = 0;
        bucket->memorySizeBytes = 0;
        bucket->ledgerSizeBytes = 0;
        bucket->memory          = nullptr;
        bucket->ledger          = nullptr;
    }

    void destruct(MemoryBucket* bucket)
    {
        wrap_destruct(&bucket->memoryMutex);
    }

    void memory_bucket_initialize(MemoryBucket* bucket, std::size_t blockSizeBytes, std::size_t blockCount, AllocatorBindings bindings)
    {
        bucket->blockSizeBytes  = blockSizeBytes;
        bucket->blockCount      = blockCount;
        bucket->memorySizeBytes = blockSizeBytes * blockCount;
        bucket->ledgerSizeBytes = 1 + ((blockCount - 1) / 8);
        bucket->memory          = bindings.allocate(bindings.allocator, bucket->memorySizeBytes);
        bucket->ledger          = bindings.allocate(bindings.allocator, bucket->ledgerSizeBytes);
        std::memset(bucket->ledger, 0, bucket->ledgerSizeBytes);
    }

    [[nodiscard]] void* memory_bucket_allocate(MemoryBucket* bucket, std::size_t memorySizeBytes)
    {
        std::lock_guard<std::mutex> lock{ bucket->memoryMutex };
        std::size_t blockNum = 1 + ((memorySizeBytes - 1) / bucket->blockSizeBytes);
        std::size_t blockId = memory_bucket_find_contiguous_blocks(bucket, blockNum);
        if (blockId == bucket->blockCount)
        {
            return nullptr;
        }
        memory_bucket_set_blocks_in_use(bucket, blockId, blockNum);
        return static_cast<uint8_t*>(bucket->memory) + blockId * bucket->blockSizeBytes;
    }

    void memory_bucket_deallocate(MemoryBucket* bucket, void* ptr, std::size_t memorySizeBytes)
    {
        std::lock_guard<std::mutex> lock{ bucket->memoryMutex };
        std::size_t blockNum = 1 + ((memorySizeBytes - 1) / bucket->blockSizeBytes);
        std::size_t blockId = (static_cast<uint8_t*>(ptr) - static_cast<uint8_t*>(bucket->memory)) / bucket->blockSizeBytes;
        memory_bucket_set_blocks_free(bucket, blockId, blockNum);
    }

    bool memory_bucket_is_belongs(MemoryBucket* bucket, void* ptr)
    {
        uint8_t* bytePtr = static_cast<uint8_t*>(ptr);
        uint8_t* byteMem = static_cast<uint8_t*>(bucket->memory);
        return (bytePtr >= byteMem) && (bytePtr < (byteMem + bucket->memorySizeBytes));
    }

    std::size_t memory_bucket_find_contiguous_blocks(MemoryBucket* bucket, std::size_t number)
    {
        std::size_t blockCounter = 0;   // contains the number of contiguous free blocks
        std::size_t currentBlockId = 0; // contains id of current block
        std::size_t firstBlockId = 0;   // contains id of the first block in the group of contiguous free blocks
        for (std::size_t it = 0; it < bucket->ledgerSizeBytes; it++)
        {
            uint8_t ledgerByte = *(static_cast<uint8_t*>(bucket->ledger) + it);
            if (ledgerByte == 255)
            {
                // @NOTE :  small optimization.
                //          We don't need to check byte if is already full.
                blockCounter = 0;
                currentBlockId += 8;
                continue;
            }
            for (std::size_t byteIt = 0; byteIt < 8; byteIt++)
            {
                if (is_bit_set(ledgerByte, byteIt))
                {
                    blockCounter = 0;
                }
                else
                {
                    if (blockCounter == 0)
                    {
                        firstBlockId = currentBlockId;
                    }
                    blockCounter++;
                }
                currentBlockId++;
                if (blockCounter == number)
                {
                    goto block_found;
                }
            }
        }
        // if nothing was found set firstBlockId to blockCount - this indicates failure of method
        firstBlockId = bucket->blockCount;
        block_found:
        return firstBlockId;
    }

    void memory_bucket_set_blocks_in_use(MemoryBucket* bucket, std::size_t first, std::size_t number)
    {
        for (std::size_t it = 0; it < number; it++)
        {
            std::size_t currentByte = (first + it) / 8;
            std::size_t currentBit = (first + it) % 8;
            uint8_t* byte = static_cast<uint8_t*>(bucket->ledger) + currentByte;
            *byte = set_bit(*byte, currentBit);
        }
    }

    void memory_bucket_set_blocks_free(MemoryBucket* bucket, std::size_t first, std::size_t number)
    {
        for (std::size_t it = 0; it < number; it++)
        {
            std::size_t currentByte = (first + it) / 8;
            std::size_t currentBit = (first + it) % 8;
            uint8_t* byte = static_cast<uint8_t*>(bucket->ledger) + currentByte;
            *byte = remove_bit(*byte, currentBit);
        }
    }

    constexpr BucketDescription memory_bucket_desc(std::size_t blockSizeBytes, std::size_t memorySizeBytes)
    {
        return { blockSizeBytes, memorySizeBytes / blockSizeBytes };
    }

    bool operator < (const BucketCompareInfo& one, const BucketCompareInfo& other)
    {
        return (one.memoryWasted == other.memoryWasted) ? one.blocksUsed < other.blocksUsed : one.memoryWasted < other.memoryWasted;
    }

    void construct(PoolAllocator* allocator, PoolAllocator::BucketDescContainer bucketDescriptions, AllocatorBindings bindings)
    {
        al_memzero(&allocator->buckets);
        al_memzero(&allocator->ptrSizePairs);
        for (std::size_t it = 0; it < EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS; it++)
        {
            construct(get(&allocator->buckets, it));
        }
        for_each_array_container(bucketDescriptions, it)
        {
            BucketDescription* desc = get(&bucketDescriptions, it);
            MemoryBucket* bucket = push(&allocator->buckets);
            memory_bucket_initialize(bucket, desc->blockSizeBytes, desc->blockCount, bindings);
        };
    }

    void destruct(PoolAllocator* allocator)
    {
        for_each_array_container(allocator->buckets, it)
        {
            destruct(get(&allocator->buckets, it));
        }
    }

    [[nodiscard]] void* allocate(PoolAllocator* allocator, std::size_t memorySizeBytes)
    {
        ArrayContainer<BucketCompareInfo, EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS> compareInfos;
        al_memzero(&compareInfos);
        std::size_t it = 0;
        for_each_array_container(allocator->buckets, it)
        {
            MemoryBucket* bucket = get(&allocator->buckets, it);
            BucketCompareInfo* info = push(&compareInfos);
            info->bucketId = it;
            if (bucket->blockSizeBytes >= memorySizeBytes)
            {
                info->blocksUsed = 1;
                info->memoryWasted = bucket->blockSizeBytes - memorySizeBytes;
            }
            else
            {
                const std::size_t blockNum = 1 + ((memorySizeBytes - 1) / bucket->blockSizeBytes);
                const std::size_t blockMemory = blockNum * bucket->blockSizeBytes;
                info->blocksUsed = blockNum;
                info->memoryWasted = blockMemory - memorySizeBytes;
            }
            it++;
        }
        // @TODO : replace std::sort mb
        std::sort(compareInfos.memory, compareInfos.memory + compareInfos.size);
        void* result = nullptr;
        for_each_array_container(compareInfos, it)
        {
            BucketCompareInfo* compareInfo = get(&compareInfos, it);
            result = memory_bucket_allocate(get(&allocator->buckets, compareInfo->bucketId), memorySizeBytes);
            if (result)
            {
                break;
            }
        }
        return result;
    }

    void deallocate(PoolAllocator* allocator, void* ptr, std::size_t memorySizeBytes)
    {
        for_each_array_container(allocator->buckets, it)
        {
            MemoryBucket* bucket = get(&allocator->buckets, it);
            if (memory_bucket_is_belongs(bucket, ptr))
            {
                memory_bucket_deallocate(bucket, ptr, memorySizeBytes);
                break;
            }
        }
    }

    [[nodiscard]] void* allocate_using_allocation_info(PoolAllocator* allocator, std::size_t memorySizeBytes)
    {
        void* ptr = allocate(allocator, memorySizeBytes);
        push(&allocator->ptrSizePairs, {
            .ptr = ptr,
            .size = memorySizeBytes
        });
        return ptr;
    }

    void deallocate_using_allocation_info(PoolAllocator* allocator, void* ptr)
    {
        for_each_dynamic_array(allocator->ptrSizePairs, it)
        {
            PoolAllocator::AllocationInfo* allocationInfo = get(&allocator->ptrSizePairs, it);
            if (allocationInfo->ptr == ptr)
            {
                deallocate(allocator, allocationInfo->ptr, allocationInfo->size);
                remove(&allocator->ptrSizePairs, it);
                break;
            }
        };
    }

    [[nodiscard]] void* reallocate_using_allocation_info(PoolAllocator* allocator, void* ptr, std::size_t newMemorySizeBytes)
    {
        void* newMemory = allocate_using_allocation_info(allocator, newMemorySizeBytes);
        for_each_dynamic_array(allocator->ptrSizePairs, it)
        {
            PoolAllocator::AllocationInfo* allocationInfo = get(&allocator->ptrSizePairs, it);
            if (allocationInfo->ptr == ptr)
            {
                std::memcpy(newMemory, allocationInfo->ptr, allocationInfo->size);
                break;
            }
        };
        deallocate_using_allocation_info(allocator, ptr);
        return newMemory;
    }

    AllocatorBindings get_allocator_bindings(PoolAllocator* pool)
    {
        return
        {
            .allocate = [](void* allocator, std::size_t size){ return allocate(static_cast<PoolAllocator*>(allocator), size); },
            .deallocate = [](void* allocator, void* ptr, std::size_t size){ deallocate(static_cast<PoolAllocator*>(allocator), ptr, size); },
            .allocator = pool
        };
    }
}
