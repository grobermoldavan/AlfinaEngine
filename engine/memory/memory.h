#ifndef AL_MEMORY_H
#define AL_MEMORY_H

#include "engine/types.h"
#include "engine/config.h"

#define al_align alignas(EngineConfig::DEFAULT_MEMORY_ALIGNMENT)

namespace al
{
    // ===============================================================================================
    // Allocator bindings data
    // ===============================================================================================

    struct AllocatorBindings
    {
        void* (*allocate)(void* allocator, uSize memorySizeBytes);
        void (*deallocate)(void* allocator, void* ptr, uSize memorySizeBytes);
        void* allocator;
    };

    // ===============================================================================================
    // Stack allocator data
    // ===============================================================================================

    struct StackAllocator
    {
        AllocatorBindings bindings;
        void* memory;
        void* memoryLimit;
        // @TODO :  add platform atomic operations
#if 0
        std::atomic<void*> top;
#else
        void* top;
#endif
    };

    // ===============================================================================================
    // Pool allocator data
    // ===============================================================================================

    struct PoolAllocatorMemoryBucket
    {
        // @TODO :  add platform mutexes
#if 0
        std::mutex memoryMutex;
#endif
        uSize blockSizeBytes;
        uSize blockCount;
        uSize memorySizeBytes;
        uSize ledgerSizeBytes;
        void* memory;
        void* ledger;
    };

    struct PoolAllocatorBucketDescription
    {
        uSize blockSizeBytes = 0;
        uSize blockCount = 0;
    };

    struct PoolAllocatorBucketCompareInfo
    {
        uSize bucketId;
        uSize blocksUsed;
        uSize memoryWasted;
    };

    bool operator < (const PoolAllocatorBucketCompareInfo& one, const PoolAllocatorBucketCompareInfo& other);

    struct PoolAllocator
    {
        AllocatorBindings bindings;
        PoolAllocatorMemoryBucket buckets[EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS];
    };

    // ===============================================================================================
    // Common interface
    // ===============================================================================================

    template<typename T> T* align_pointer(T* ptr);
    AllocatorBindings get_system_allocator_bindings();

    void*   allocate    (AllocatorBindings* bindings, uSize memorySizeBytes);
    void    deallocate  (AllocatorBindings* bindings, void* ptr, uSize memorySizeBytes);

    // ===============================================================================================
    // Stack allocator interface
    // ===============================================================================================

    void    construct   (StackAllocator* stack, uSize memorySizeBytes, AllocatorBindings bindings);
    void    destruct    (StackAllocator* stack);
    void*   allocate    (StackAllocator* stack, uSize memorySizeBytes);
    void    deallocate  (StackAllocator* stack, void* ptr, uSize memorySizeBytes);

    AllocatorBindings get_allocator_bindings(StackAllocator* stack);

    // ===============================================================================================
    // Pool allocator interface
    // ===============================================================================================

    constexpr PoolAllocatorBucketDescription memory_bucket_desc(uSize blockSizeBytes, uSize memorySizeBytes);

    void    memory_bucket_construct             (PoolAllocatorMemoryBucket* bucket, uSize blockSizeBytes, uSize blockCount, AllocatorBindings bindings);
    void    memory_bucket_destruct              (PoolAllocatorMemoryBucket* bucket, AllocatorBindings bindings);
    void*   memory_bucket_allocate              (PoolAllocatorMemoryBucket* bucket, uSize memorySizeBytes);
    void    memory_bucket_deallocate            (PoolAllocatorMemoryBucket* bucket, void* ptr, uSize memorySizeBytes);
    bool    memory_bucket_is_belongs            (PoolAllocatorMemoryBucket* bucket, void* ptr);
    uSize   memory_bucket_find_contiguous_blocks(PoolAllocatorMemoryBucket* bucket, uSize number);
    void    memory_bucket_set_blocks_in_use     (PoolAllocatorMemoryBucket* bucket, uSize first, uSize number);
    void    memory_bucket_set_blocks_free       (PoolAllocatorMemoryBucket* bucket, uSize first, uSize number);

    void    construct   (PoolAllocator* allocator, const PoolAllocatorBucketDescription bucketDescriptions[EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS], AllocatorBindings bindings);
    void    destruct    (PoolAllocator* allocator);
    void*   allocate    (PoolAllocator* allocator, uSize memorySizeBytes);
    void    deallocate  (PoolAllocator* allocator, void* ptr, uSize memorySizeBytes);

    AllocatorBindings get_allocator_bindings(PoolAllocator* pool);
}

#endif
