
#include <cstring>
#include <cstdlib>
#include <algorithm>

#include "engine/memory/memory.h"

namespace al
{
    template<typename T>
    T* align_pointer(T* ptr, uSize alignment)
    {
        uPtr uintPtr = reinterpret_cast<uPtr>(ptr);
        u64 diff = uintPtr & (alignment - 1);
        if (diff != 0)
        {
            diff = alignment - diff;
        }
        u8* alignedPtr = reinterpret_cast<u8*>(ptr) + diff;
        return reinterpret_cast<T*>(alignedPtr);
    }

    AllocatorBindings get_system_allocator_bindings()
    {
        return
        {
            .allocate = [](void* allocator, uSize size, uSize alignment){ return al_aligned_system_malloc(size, alignment); },
            .deallocate = [](void* allocator, void* ptr, uSize size){ al_aligned_system_free(ptr); },
            .allocator = nullptr
        };
    }

    template<typename T>
    T* allocate(AllocatorBindings* bindings, uSize amount, uSize alignment)
    {
        al_check_alignment(alignment);
        if constexpr (std::is_same_v<T, void>)
        {
            return bindings->allocate(bindings->allocator, amount, alignment);
        }
        else
        {
            return (T*)allocate<void>(bindings, amount * sizeof(T));
        }
    }

    template<typename T>
    void deallocate(AllocatorBindings* bindings, T* ptr, uSize amount)
    {
        if constexpr (std::is_same_v<T, void>)
        {
            bindings->deallocate(bindings->allocator, ptr, amount);
        }
        else
        {
            deallocate<void>(bindings, ptr, amount * sizeof(T));
        }
    }

    void construct(StackAllocator* stack, uSize memorySizeBytes, AllocatorBindings* bindings)
    {
        stack->bindings = *bindings;
        stack->memory = allocate(bindings, memorySizeBytes, EngineConfig::MAX_MEMORY_ALIGNMENT);
        stack->memoryLimit = static_cast<u8*>(stack->memory) + memorySizeBytes;
        stack->top = stack->memory;
    }

    void destruct(StackAllocator* stack)
    {
        uSize stackSize = static_cast<u8*>(stack->memoryLimit) - static_cast<u8*>(stack->memory);
        stack->bindings.deallocate(stack->bindings.allocator, stack->memory, stackSize);
    }

    void* allocate(StackAllocator* stack, uSize memorySizeBytes, uSize alignment)
    {
        al_check_alignment(alignment);
        // @TODO :  add platform atomic operations
#if 0
        void* result = nullptr;
        while (true)
        {
            void* currentTop = std::atomic_load_explicit(&stack->top, std::memory_order_relaxed);
            u8* currentTopAligned = align_pointer(static_cast<u8*>(currentTop), alignment);
            if ((static_cast<u8*>(stack->memoryLimit) - currentTopAligned) < memorySizeBytes)
            {
                break;
            }
            u8* newTop = currentTopAligned + memorySizeBytes;
            const bool casResult = std::atomic_compare_exchange_strong(&stack->top, &currentTop, newTop);
            if (casResult)
            {
                result = currentTopAligned;
                break;
            }
        }
        return result;
#else
        u8* currentTopAligned = align_pointer(static_cast<u8*>(stack->top), alignment);
        if ((static_cast<u8*>(stack->memoryLimit) - currentTopAligned) < memorySizeBytes)
        {
            return nullptr;
        }
        return currentTopAligned + memorySizeBytes;
#endif
    }

    void deallocate(StackAllocator* stack, void* ptr, uSize memorySizeBytes)
    {
        // nothing
    }

    AllocatorBindings get_allocator_bindings(StackAllocator* stack)
    {
        return
        {
            .allocate = [](void* allocator, uSize size, uSize alignment){ return allocate(static_cast<StackAllocator*>(allocator), size, alignment); },
            .deallocate = [](void* allocator, void* ptr, uSize size){ deallocate(static_cast<StackAllocator*>(allocator), ptr, size); },
            .allocator = stack
        };
    }

    constexpr PoolAllocatorBucketDescription memory_bucket_desc(uSize blockSizeBytes, uSize memorySizeBytes)
    {
        return { blockSizeBytes, memorySizeBytes / blockSizeBytes };
    }

    void memory_bucket_construct(PoolAllocatorMemoryBucket* bucket, uSize blockSizeBytes, uSize blockCount, AllocatorBindings* bindings)
    {
        bucket->blockSizeBytes  = blockSizeBytes;
        bucket->blockCount      = blockCount;
        bucket->memorySizeBytes = blockSizeBytes * blockCount;
        bucket->ledgerSizeBytes = 1 + ((blockCount - 1) / 8);
        bucket->memory          = allocate(bindings, bucket->memorySizeBytes, EngineConfig::MAX_MEMORY_ALIGNMENT);
        bucket->ledger          = allocate(bindings, bucket->ledgerSizeBytes);
        std::memset(bucket->ledger, 0, bucket->ledgerSizeBytes);
    }

    void memory_bucket_destruct(PoolAllocatorMemoryBucket* bucket, AllocatorBindings* bindings)
    {
        deallocate(bindings, bucket->memory, bucket->memorySizeBytes);
        deallocate(bindings, bucket->ledger, bucket->ledgerSizeBytes);
    }

    void* memory_bucket_allocate(PoolAllocatorMemoryBucket* bucket, uSize memorySizeBytes, uSize alignment)
    {
        al_check_alignment(alignment);
        // std::lock_guard<std::mutex> lock{ bucket->memoryMutex };
        uSize blockNum = 1 + ((memorySizeBytes - 1) / bucket->blockSizeBytes);
        uSize blockId = memory_bucket_find_contiguous_blocks(bucket, blockNum, alignment);
        if (blockId == bucket->blockCount)
        {
            return nullptr;
        }
        memory_bucket_set_blocks_in_use(bucket, blockId, blockNum);
        return static_cast<u8*>(bucket->memory) + blockId * bucket->blockSizeBytes;
    }

    void memory_bucket_deallocate(PoolAllocatorMemoryBucket* bucket, void* ptr, uSize memorySizeBytes)
    {
        // std::lock_guard<std::mutex> lock{ bucket->memoryMutex };
        uSize blockNum = 1 + ((memorySizeBytes - 1) / bucket->blockSizeBytes);
        uSize blockId = (static_cast<u8*>(ptr) - static_cast<u8*>(bucket->memory)) / bucket->blockSizeBytes;
        memory_bucket_set_blocks_free(bucket, blockId, blockNum);
    }

    bool memory_bucket_is_belongs(PoolAllocatorMemoryBucket* bucket, void* ptr)
    {
        u8* bytePtr = static_cast<u8*>(ptr);
        u8* byteMem = static_cast<u8*>(bucket->memory);
        return (bytePtr >= byteMem) && (bytePtr < (byteMem + bucket->memorySizeBytes));
    }

    uSize memory_bucket_find_contiguous_blocks(PoolAllocatorMemoryBucket* bucket, uSize number, uSize alignment)
    {
        auto isBitSet = [](u8 value, uSize bit) -> u8 { return ((value >> bit) & 1) == 1; };
        uSize blockCounter = 0;   // contains the number of contiguous free blocks
        uSize currentBlockId = 0; // contains id of current block
        uSize firstBlockId = 0;   // contains id of the first block in the group of contiguous free blocks
        for (uSize it = 0; it < bucket->ledgerSizeBytes; it++)
        {
            u8 ledgerByte = *(static_cast<u8*>(bucket->ledger) + it);
            if (ledgerByte == 255)
            {
                // Small optimization. We don't need to check byte if is already full.
                blockCounter = 0;
                currentBlockId += 8;
                continue;
            }
            for (uSize byteIt = 0; byteIt < 8; byteIt++)
            {
                if (isBitSet(ledgerByte, byteIt))
                {
                    blockCounter = 0;
                }
                else
                {
                    if (blockCounter == 0)
                    {
                        uPtr currentBlockAddress = reinterpret_cast<uPtr>(bucket->memory) + (it * 8 + byteIt) * bucket->blockSizeBytes;
                        bool isCorrectFirstBlock = (currentBlockAddress % alignment) == 0;
                        if (isCorrectFirstBlock)
                        {
                            firstBlockId = currentBlockId;
                            blockCounter++;
                        }
                    }
                    else
                    {
                        blockCounter++;
                    }
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

    void memory_bucket_set_blocks_in_use(PoolAllocatorMemoryBucket* bucket, uSize first, uSize number)
    {
        auto setBit = [](u8 value, uSize bit) -> u8
        {
            return value | (1 << bit);
        };
        for (uSize it = 0; it < number; it++)
        {
            uSize currentByte = (first + it) / 8;
            uSize currentBit = (first + it) % 8;
            u8* byte = static_cast<u8*>(bucket->ledger) + currentByte;
            *byte = setBit(*byte, currentBit);
        }
    }

    void memory_bucket_set_blocks_free(PoolAllocatorMemoryBucket* bucket, uSize first, uSize number)
    {
        auto removeBit = [](u8 value, uSize bit) -> u8
        {
            return value & ~(1 << bit);
        };
        for (uSize it = 0; it < number; it++)
        {
            uSize currentByte = (first + it) / 8;
            uSize currentBit = (first + it) % 8;
            u8* byte = static_cast<u8*>(bucket->ledger) + currentByte;
            *byte = removeBit(*byte, currentBit);
        }
    }

    bool operator < (const PoolAllocatorBucketCompareInfo& one, const PoolAllocatorBucketCompareInfo& other)
    {
        return (one.memoryWasted == other.memoryWasted) ? one.blocksUsed < other.blocksUsed : one.memoryWasted < other.memoryWasted;
    }

    void construct(PoolAllocator* allocator, const PoolAllocatorBucketDescription bucketDescriptions[EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS], AllocatorBindings* bindings)
    {
        allocator->bindings = *bindings;
        std::memset(&allocator->buckets, 0, sizeof(decltype(allocator->buckets)));
        for (uSize it = 0; it < EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS; it++)
        {
            const PoolAllocatorBucketDescription* desc = &bucketDescriptions[it];
            if (desc->blockSizeBytes == 0 || desc->blockCount == 0)
            {
                break;
            }
            PoolAllocatorMemoryBucket* bucket = &allocator->buckets[it];
            memory_bucket_construct(bucket, desc->blockSizeBytes, desc->blockCount, bindings);
        }
    }

    void destruct(PoolAllocator* allocator)
    {
        for (uSize it = 0; it < EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS; it++)
        {
            PoolAllocatorMemoryBucket* bucket = &allocator->buckets[it];
            if (!bucket->memory)
            {
                break;
            }
            memory_bucket_destruct(bucket, &allocator->bindings);
        }
    }

    void* allocate(PoolAllocator* allocator, uSize memorySizeBytes, uSize alignment)
    {
        al_check_alignment(alignment);
        PoolAllocatorBucketCompareInfo compareInfos[EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS] = {};
        uSize it;
        // @NOTE :  this bucket comparison doesn't take into account an alignment of allocation
        for (it = 0; it < EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS; it++)
        {
            PoolAllocatorMemoryBucket* bucket = &allocator->buckets[it];
            if (!bucket->memory)
            {
                break;
            }
            PoolAllocatorBucketCompareInfo* info = &compareInfos[it];
            info->bucketId = it;
            if (bucket->blockSizeBytes >= memorySizeBytes)
            {
                info->blocksUsed = 1;
                info->memoryWasted = bucket->blockSizeBytes - memorySizeBytes;
            }
            else
            {
                uSize blockNum = 1 + ((memorySizeBytes - 1) / bucket->blockSizeBytes);
                uSize blockMemory = blockNum * bucket->blockSizeBytes;
                info->blocksUsed = blockNum;
                info->memoryWasted = blockMemory - memorySizeBytes;
            }
        }
        std::sort(compareInfos, compareInfos + it);
        void* result = nullptr;
        for (it = 0; it < EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS; it++)
        {
            PoolAllocatorBucketCompareInfo* info = &compareInfos[it];
            if (info->blocksUsed == 0)
            {
                break;
            }
            result = memory_bucket_allocate(&allocator->buckets[info->bucketId], memorySizeBytes, alignment);
            if (result)
            {
                break;
            }
        }
        return result;
    }

    void deallocate(PoolAllocator* allocator, void* ptr, uSize memorySizeBytes)
    {
        for (uSize it = 0; it < EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS; it++)
        {
            PoolAllocatorMemoryBucket* bucket = &allocator->buckets[it];
            if (!bucket->memory)
            {
                break;
            }
            if (memory_bucket_is_belongs(bucket, ptr))
            {
                memory_bucket_deallocate(bucket, ptr, memorySizeBytes);
                break;
            }
        }
    }

    AllocatorBindings get_allocator_bindings(PoolAllocator* pool)
    {
        return
        {
            .allocate = [](void* allocator, uSize size, uSize alignment){ return allocate(static_cast<PoolAllocator*>(allocator), size, alignment); },
            .deallocate = [](void* allocator, void* ptr, uSize size){ deallocate(static_cast<PoolAllocator*>(allocator), ptr, size); },
            .allocator = pool
        };
    }
}
