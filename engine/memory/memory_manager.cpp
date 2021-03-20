
#include "memory_manager.h"
#include "memory_common.h"
#include "engine/config/engine_config.h"
#include "engine/debug/debug.h"

#include "utilities/constexpr_functions.h"

namespace al::engine
{
    extern MemoryManager* gMemoryManager = nullptr;

    void construct(MemoryManager* manager)
    {
        manager->memory = std::malloc(EngineConfig::MEMORY_SIZE + EngineConfig::DEFAULT_MEMORY_ALIGNMENT);
        construct(&manager->stack, align_pointer(static_cast<uint8_t*>(manager->memory)), EngineConfig::MEMORY_SIZE);
        {
            auto alignBucketSize = [](std::size_t targetSize, std::size_t blockSize) -> std::size_t
            {
                return targetSize % blockSize == 0 ? targetSize : targetSize + blockSize - (targetSize % blockSize);
            };
            std::size_t bucketSize1 = alignBucketSize(percent_of<std::size_t>(EngineConfig::POOL_ALLOCATOR_MEMORY_SIZE, 10), kilobytes<std::size_t>(1));
            std::size_t bucketSize2 = alignBucketSize(percent_of<std::size_t>(EngineConfig::POOL_ALLOCATOR_MEMORY_SIZE, 20), 128);
            std::size_t bucketSize3 = alignBucketSize(percent_of<std::size_t>(EngineConfig::POOL_ALLOCATOR_MEMORY_SIZE, 30), 16);
            std::size_t bucketSize4 = EngineConfig::POOL_ALLOCATOR_MEMORY_SIZE - (bucketSize1 + bucketSize2 + bucketSize3);
            PoolAllocator::BucketDescContainer poolContainer;
            al_memzero(&poolContainer);
            push(&poolContainer, memory_bucket_desc(kilobytes<std::size_t>(1)   , bucketSize1));
            push(&poolContainer, memory_bucket_desc(128                         , bucketSize2));
            push(&poolContainer, memory_bucket_desc(16                          , bucketSize3));
            push(&poolContainer, memory_bucket_desc(8                           , bucketSize4));
            construct(&manager->pool, poolContainer, get_allocator_bindings(&manager->stack));
        }
    }

    void destruct(MemoryManager* manager)
    {
        destruct(&manager->stack);
        destruct(&manager->pool);
        std::free(manager->memory);
    }

    // void MemoryManager::log_memory_init_info() noexcept
    // {
    //     al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, "Allocated %d bytes of memory. %d bytes are used for alignment", EngineConfig::MEMORY_SIZE + EngineConfig::DEFAULT_MEMORY_ALIGNMENT, EngineConfig::DEFAULT_MEMORY_ALIGNMENT);
    //     al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, "Pool allocator uses %d bytes of memory", EngineConfig::POOL_ALLOCATOR_MEMORY_SIZE);
    //     al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, "Pool allocator buckets info : ");
    //     PoolAllocator::BucketContainer& buckets = manager->pool.get_buckets();
    //     std::size_t it = 0;
    //     std::size_t totalBucketsMemorySize = 0;
    //     for_each_array_container(buckets, it)
    //     {
    //         MemoryBucket* bucket = get(&buckets, it);
    //         std::size_t bucketSize = bucket->get_block_size() * bucket->get_block_count();
    //         totalBucketsMemorySize += bucketSize;
    //         al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, 
    //                         "    Bucket %d : block size is %5d bytes, block count is %10d, uses %10d bytes of memory in total", 
    //                         it, bucket->get_block_size(), bucket->get_block_count(), bucketSize);
    //         it++;
    //     }
    //     // @NOTE :  This is probably possible, that memory can be wasted (not used in any of the memory buckets) with some specific pool allocator configurations
    //     al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, "Pool allocator : total memory wasted : %d bytes", EngineConfig::POOL_ALLOCATOR_MEMORY_SIZE - totalBucketsMemorySize);
    //     al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, "ECS Pool allocator uses %d bytes of memory", EngineConfig::ECS_POOL_ALLOCATOR_MEMORY_SIZE);
    //     // @NOTE :  ECS pool currently uses only one bucket
    //     MemoryBucket* ecsBucket = get(&get_ecs_pool()->get_buckets(), 0);
    //     std::size_t ecsBucketSize = ecsBucket->get_block_size() * ecsBucket->get_block_count();
    //     al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, "ECS Pool allocator : total memory waster : %d bytes", EngineConfig::ECS_POOL_ALLOCATOR_MEMORY_SIZE - ecsBucketSize);
    // }
}
