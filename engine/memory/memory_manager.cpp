
#include "memory_manager.h"
#include "memory_common.h"
#include "engine/config/engine_config.h"
#include "engine/debug/debug.h"

#include "utilities/constexpr_functions.h"

namespace al::engine
{
    MemoryManager MemoryManager::instance{ };

    MemoryManager::MemoryManager() noexcept
        : memory{ nullptr }
    { }

    MemoryManager::~MemoryManager() noexcept
    { }

    void MemoryManager::construct() noexcept
    {
        if (instance.memory)
        {
            return;
        }
        instance.memory = static_cast<std::byte*>(std::malloc(EngineConfig::MEMORY_SIZE + EngineConfig::DEFAULT_MEMORY_ALIGNMENT));
        instance.stack.initialize(align_pointer(instance.memory), EngineConfig::MEMORY_SIZE);
        std::size_t poolMemorySizeBytes = percent_of<std::size_t>(EngineConfig::MEMORY_SIZE, EngineConfig::POOL_ALLOCATOR_MEMORY_PERCENTS);
        auto alignBucketSize = [](std::size_t targetSize, std::size_t blockSize) -> std::size_t
        {
            return targetSize % blockSize == 0 ? targetSize : targetSize + blockSize - (targetSize % blockSize);
        };
        std::size_t bucketSize1 = alignBucketSize(percent_of<std::size_t>(poolMemorySizeBytes, 10), kilobytes<std::size_t>(1));
        std::size_t bucketSize2 = alignBucketSize(percent_of<std::size_t>(poolMemorySizeBytes, 20), 128);
        std::size_t bucketSize3 = alignBucketSize(percent_of<std::size_t>(poolMemorySizeBytes, 30), 16);
        std::size_t bucketSize4 = poolMemorySizeBytes - (bucketSize1 + bucketSize2 + bucketSize3);
        instance.pool.initialize({
            bucket_desc(kilobytes<std::size_t>(1)   , bucketSize1),
            bucket_desc(128                         , bucketSize2),
            bucket_desc(16                          , bucketSize3),
            bucket_desc(8                           , bucketSize4),
        }, &instance.stack);
    }

    void MemoryManager::destruct() noexcept
    {
        if (!instance.memory)
        {
            return;
        }
        std::free(instance.memory);
        instance.~MemoryManager();
    }

    inline StackAllocator* MemoryManager::get_stack() noexcept
    {
        return &instance.stack;
    }

    inline PoolAllocator* MemoryManager::get_pool() noexcept
    {
        return &instance.pool;
    }

    void MemoryManager::log_memory_init_info() noexcept
    {
        std::size_t poolMemorySizeBytes = percent_of<std::size_t>(EngineConfig::MEMORY_SIZE, EngineConfig::POOL_ALLOCATOR_MEMORY_PERCENTS);
        al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, "Allocated %d bytes of memory. %d bytes are used for alignment", EngineConfig::MEMORY_SIZE + EngineConfig::DEFAULT_MEMORY_ALIGNMENT, EngineConfig::DEFAULT_MEMORY_ALIGNMENT);
        al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, "Pool allocator uses %d bytes of memory", poolMemorySizeBytes);
        al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, "Pool allocator buckets info : ");
        PoolAllocator::BucketContainer& buckets = instance.pool.get_buckets();
        std::size_t it = 0;
        std::size_t totalBucketsMemorySize = 0;
        for (MemoryBucket& bucket : buckets)
        {
            std::size_t bucketSize = bucket.get_block_size() * bucket.get_block_count();
            totalBucketsMemorySize += bucketSize;
            al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, 
                            "    Bucket %d : block size is %5d bytes, block count is %10d, uses %10d bytes of memory in total", 
                            it, bucket.get_block_size(), bucket.get_block_count(), bucketSize);
            it++;
        }
        // @NOTE : This is probably possible, that memory can be wasted (not used in any of the memory buckets) with some specific pool allocator configurations
        al_log_message(EngineConfig::MEMORY_MANAGER_LOG_CATEGORY, "Pool allocator : total memory wasted : %d bytes", poolMemorySizeBytes - totalBucketsMemorySize);
    }
}
