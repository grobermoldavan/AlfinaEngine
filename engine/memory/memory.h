#ifndef AL_MEMORY_H
#define AL_MEMORY_H

#include <cstdlib>

#include "allocator_bindings.h"
#include "engine/types.h"
#include "engine/config.h"
#include "engine/debug/assert.h"

#define al_align                        alignas(EngineConfig::DEFAULT_MEMORY_ALIGNMENT)
#define al_check_alignment(alignment)   al_assert_msg(((alignment - 1) & alignment) == 0, "Alignment must be a power of two"); \
                                        al_assert_msg(alignment <= EngineConfig::MAX_MEMORY_ALIGNMENT, "Requested alignment is too big - consider updating EngineConfig::MAX_MEMORY_ALIGNMENT value")

#ifdef _MSC_VER
#   define al_aligned_system_malloc(size, alignment) _aligned_malloc(size, alignment)
#   define al_aligned_system_free(ptr) _aligned_free(ptr)
#else
#   define al_aligned_system_malloc(size, alignment) std::aligned_alloc(size, alignment)
#   define al_aligned_system_free(ptr) std::free(ptr)
#endif

namespace al
{
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

    template<uSize SizeBytes>
    struct InplaceStackAllocator
    {
        u8 memory[SizeBytes];
        uSize top;
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

    template<typename T> T* align_pointer(T* ptr, uSize alignment = EngineConfig::DEFAULT_MEMORY_ALIGNMENT);
    AllocatorBindings get_system_allocator_bindings();

    template<typename T = void> T*      allocate    (AllocatorBindings* bindings, uSize amount = 1, uSize alignment = EngineConfig::DEFAULT_MEMORY_ALIGNMENT);
    template<typename T = void> void    deallocate  (AllocatorBindings* bindings, T* ptr, uSize amount = 1);

    template<typename Allocator>
    AllocatorBindings get_allocator_bindings(Allocator* allocator);

    // ===============================================================================================
    // Stack allocator interface
    // ===============================================================================================

    void stack_alloactor_reset(StackAllocator* stack);

    void    construct   (StackAllocator* stack, uSize memorySizeBytes, AllocatorBindings* bindings);
    void    destruct    (StackAllocator* stack);
    void*   allocate    (StackAllocator* stack, uSize memorySizeBytes, uSize alignment);
    void    deallocate  (StackAllocator* stack, void* ptr, uSize memorySizeBytes);
    template<uSize SizeBytes> void*   allocate    (InplaceStackAllocator<SizeBytes>* stack, uSize memorySizeBytes, uSize alignment);
    template<uSize SizeBytes> void    deallocate  (InplaceStackAllocator<SizeBytes>* stack, void* ptr, uSize memorySizeBytes);

    // ===============================================================================================
    // Pool allocator interface
    // ===============================================================================================

    constexpr PoolAllocatorBucketDescription memory_bucket_desc(uSize blockSizeBytes, uSize memorySizeBytes);

    void    memory_bucket_construct             (PoolAllocatorMemoryBucket* bucket, uSize blockSizeBytes, uSize blockCount, AllocatorBindings* bindings);
    void    memory_bucket_destruct              (PoolAllocatorMemoryBucket* bucket, AllocatorBindings* bindings);
    void*   memory_bucket_allocate              (PoolAllocatorMemoryBucket* bucket, uSize memorySizeBytes, uSize alignment);
    void    memory_bucket_deallocate            (PoolAllocatorMemoryBucket* bucket, void* ptr, uSize memorySizeBytes);
    bool    memory_bucket_is_belongs            (PoolAllocatorMemoryBucket* bucket, void* ptr);
    uSize   memory_bucket_find_contiguous_blocks(PoolAllocatorMemoryBucket* bucket, uSize number, uSize alignment);
    void    memory_bucket_set_blocks_in_use     (PoolAllocatorMemoryBucket* bucket, uSize first, uSize number);
    void    memory_bucket_set_blocks_free       (PoolAllocatorMemoryBucket* bucket, uSize first, uSize number);

    void    construct   (PoolAllocator* allocator, const PoolAllocatorBucketDescription bucketDescriptions[EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS], AllocatorBindings* bindings);
    void    destruct    (PoolAllocator* allocator);
    void*   allocate    (PoolAllocator* allocator, uSize memorySizeBytes, uSize alignment);
    void    deallocate  (PoolAllocator* allocator, void* ptr, uSize memorySizeBytes);
}

#endif
