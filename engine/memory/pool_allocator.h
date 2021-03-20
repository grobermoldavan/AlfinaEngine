#ifndef AL_POOL_ALLOCATOR_H
#define AL_POOL_ALLOCATOR_H

#include <cstddef>  // for std::size_t
#include <mutex>    // for std::mutex

#include "allocator_base.h"
#include "engine/config/engine_config.h"

#include "utilities/constexpr_functions.h"
#include "utilities/array_container.h"

// @NOTE :  This allocator is thread-safe because of mutexes

// @NOTE :  This allocator implementation is based on Misha Shalem's talk 
//          "Practical Memory Pool Based Allocators For Modern C++" on CppCon 2020
//          https://www.youtube.com/watch?v=l14Zkx5OXr4

namespace al::engine
{
    struct MemoryBucket
    {
        std::mutex memoryMutex;
        std::size_t blockSizeBytes;
        std::size_t blockCount;
        std::size_t memorySizeBytes;
        std::size_t ledgerSizeBytes;
        void* memory;
        void* ledger;
    };

    void                construct   (MemoryBucket* bucket);
    void                destruct    (MemoryBucket* bucket);
    
    void                memory_bucket_initialize            (MemoryBucket* bucket, std::size_t blockSizeBytes, std::size_t blockCount, AllocatorBindings bindings);
    [[nodiscard]] void* memory_bucket_allocate              (MemoryBucket* bucket, std::size_t memorySizeBytes);
    void                memory_bucket_deallocate            (MemoryBucket* bucket, void* ptr, std::size_t memorySizeBytes);
    bool                memory_bucket_is_belongs            (MemoryBucket* bucket, void* ptr);
    std::size_t         memory_bucket_find_contiguous_blocks(MemoryBucket* bucket, std::size_t number);
    void                memory_bucket_set_blocks_in_use     (MemoryBucket* bucket, std::size_t first, std::size_t number);
    void                memory_bucket_set_blocks_free       (MemoryBucket* bucket, std::size_t first, std::size_t number);

    struct BucketDescription
    {
        std::size_t blockSizeBytes = 0;
        std::size_t blockCount = 0;
    };

    constexpr BucketDescription memory_bucket_desc(std::size_t blockSizeBytes, std::size_t memorySizeBytes);

    struct BucketCompareInfo
    {
        std::size_t bucketId;
        std::size_t blocksUsed;
        std::size_t memoryWasted;
    };

    bool operator < (const BucketCompareInfo& one, const BucketCompareInfo& other);

    struct PoolAllocator
    {
        using BucketDescContainer = ArrayContainer<BucketDescription, EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS>;
        using BucketContainer = ArrayContainer<MemoryBucket, EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS>;
        struct AllocationInfo
        {
            void* ptr;
            std::size_t size;
        };
        BucketContainer buckets;
        ArrayContainer<AllocationInfo, EngineConfig::POOL_ALLOCATOR_MAX_PTR_SIZE_PAIRS> ptrSizePairs;
    };

    void                construct   (PoolAllocator* allocator, PoolAllocator::BucketDescContainer bucketDescriptions, AllocatorBindings bindings);
    void                destruct    (PoolAllocator* allocator);

    [[nodiscard]] void* allocate    (PoolAllocator* allocator, std::size_t memorySizeBytes);
    void                deallocate  (PoolAllocator* allocator, void* ptr, std::size_t memorySizeBytes);

    // @NOTE :  This methods allow user to deallocate and reallocate memory using only memory pointer without passing memory size.
    //          This might be useful for connecting allocator to other API's. For example, this methods are currently used with stbi_image
    //          (engine/platform/win32/opengl/win32_opengl_backend.h).
    //          This methods use AllocationInfo struct to store ptr to allocated data as well as allocated size. All this info stored in
    //          ptrSizePairs.
    //          You CAN'T succsessfully use deallocate_using_allocation_info or reallocate_using_allocation_info if memory was not allocated
    //          via allocate_using_allocation_info method.
    [[nodiscard]] void* allocate_using_allocation_info  (PoolAllocator* allocator, std::size_t memorySizeBytes);
    void                deallocate_using_allocation_info(PoolAllocator* allocator, void* ptr);
    [[nodiscard]] void* reallocate_using_allocation_info(PoolAllocator* allocator, void* ptr, std::size_t newMemorySizeBytes);

    AllocatorBindings get_allocator_bindings(PoolAllocator* pool);
}

#endif
