#ifndef AL_POOL_ALLOCATOR_H
#define AL_POOL_ALLOCATOR_H

#define POOL_ALLOCATOR_USE_LOCK 1

#include <cstddef>
#include <cstring>
#include <array>
#include <algorithm>
#if(POOL_ALLOCATOR_USE_LOCK)
#   include <mutex>
#endif

#include "allocator_base.h"
#include "engine/config/engine_config.h"

#include "utilities/constexpr_functions.h"
#include "utilities/array_container.h"

// @NOTE :  This allocator is thread-safe if POOL_ALLOCATOR_USE_LOCK is true

// @NOTE :  This allocator implementation is based on Misha Shalem's talk 
//          "Practical Memory Pool Based Allocators For Modern C++" on CppCon 2020
//          https://www.youtube.com/watch?v=l14Zkx5OXr4

namespace al::engine
{
    class MemoryBucket
    {
    public:
        MemoryBucket() noexcept;
        ~MemoryBucket() noexcept;

        void initialize(std::size_t blockSize, std::size_t blockCount, AllocatorBase* allocator) noexcept;
        [[nodiscard]] std::byte* allocate(std::size_t memorySizeBytes) noexcept;
        void deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept;
        const bool is_belongs(std::byte* ptr) const noexcept;
        const std::size_t get_block_size() const noexcept;
        const std::size_t get_block_count() const noexcept;
        const bool is_bucket_initialized() const noexcept;

        const std::byte* get_ledger() const noexcept;
        const std::size_t get_ledger_size_bytes() const noexcept;

    private:
#if(POOL_ALLOCATOR_USE_LOCK)
        std::mutex memoryMutex;
#endif

        std::size_t blockSize;
        std::size_t blockCount;

        std::size_t memorySizeBytes;
        std::size_t ledgerSizeBytes;

        std::byte* memory;
        std::byte* ledger;

        std::size_t find_contiguous_blocks(std::size_t number) const noexcept;
        void set_blocks_in_use(std::size_t first, std::size_t number) noexcept;
        void set_blocks_free(std::size_t first, std::size_t number) noexcept;
    };

    struct BucketDescrition
    {
        std::size_t blockSize = 0;
        std::size_t blockCount = 0;
    };

    constexpr BucketDescrition bucket_desc(std::size_t blockSizeBytes, std::size_t memorySizeBytes);

    struct BucketCompareInfo
    {
        std::size_t bucketId;
        std::size_t blocksUsed;
        std::size_t memoryWasted;

        bool operator < (const BucketCompareInfo& other) const noexcept;
    };

    class PoolAllocator : public AllocatorBase
    {
    public:
        PoolAllocator() = default;
        ~PoolAllocator() = default;

        virtual [[nodiscard]] std::byte* allocate(std::size_t memorySizeBytes) noexcept override;
        virtual void deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept override;

        void initialize(std::array<BucketDescrition, EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS> bucketDescriptions, AllocatorBase* allocator) noexcept;

        // @NOTE :  This methods allow user to deallocate and reallocate memory using only memory pointer without passing memory size.
        //          This might be useful for connecting allocator to other API's. For example, this methods are currently used with stbi_image
        //          (engine/platform/win32/opengl/win32_opengl_backend.h).
        //          This methods use AllocationInfo struct to store ptr to allocated data as well as allocated size. All this info stored in
        //          ptrSizePairs.
        //          You CAN'T succsessfully use deallocate_using_allocation_info or reallocate_using_allocation_info if memory was not allocated
        //          via allocate_using_allocation_info method.
        [[nodiscard]] std::byte* allocate_using_allocation_info(std::size_t memorySizeBytes) noexcept;
        void deallocate_using_allocation_info(std::byte* ptr) noexcept;
        [[nodiscard]] std::byte* reallocate_using_allocation_info(std::byte* ptr, std::size_t newMemorySizeBytes) noexcept;

    private:
        std::array<MemoryBucket, EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS> buckets;

        struct AllocationInfo
        {
            std::byte* ptr;
            std::size_t size;
        };

        ArrayContainer<AllocationInfo, EngineConfig::POOL_ALLOCATOR_MAX_PTR_SIZE_PAIRS> ptrSizePairs;
    };

    // ==========================================================================================================================================
    // @NOTE : This is a wrapper over pool allocator which gives us an ability to use custom allocator with std containers
    // ==========================================================================================================================================

    template<typename T>
    class PoolAllocatorStdWrap
    {
    public:
        using value_type = T;

        PoolAllocatorStdWrap() noexcept
            : allocator { MemoryManager::get()->get_pool() }
        { }

        template<typename U>
        PoolAllocatorStdWrap(const PoolAllocatorStdWrap<U>&) noexcept
            : allocator { MemoryManager::get()->get_pool() }
        { }

        ~PoolAllocatorStdWrap() = default;

        T* allocate(std::size_t n)
        {
            return reinterpret_cast<T*>(allocator->allocate(n * sizeof(T)));
        }

        void deallocate(T* ptr, std::size_t n) 
        {
            allocator->deallocate((std::byte*)ptr, n * sizeof(T));
        }
    
    private:
        PoolAllocator* allocator;
    };

    template <typename T, typename U>
    constexpr bool operator == (const PoolAllocatorStdWrap<T>&, const PoolAllocatorStdWrap<U>&) noexcept
    {
        return true;
    }

    template <typename T, typename U>
    constexpr bool operator != (const PoolAllocatorStdWrap<T>&, const PoolAllocatorStdWrap<U>&) noexcept
    {
        return false;
    }
}

#endif
