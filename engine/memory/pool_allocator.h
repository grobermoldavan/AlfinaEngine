#ifndef AL_POOL_ALLOCATOR_H
#define AL_POOL_ALLOCATOR_H

#include <cstddef>
#include <cstring>
#include <array>
#include <algorithm>

#include "allocator_base.h"

#include "utilities/constexpr_functions.h"

// @NOTE :  This allocator is not thread-safe

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
        const bool is_initialized() const noexcept;

    private:
        std::size_t find_contiguous_blocks(std::size_t number) const noexcept;
        void set_blocks_in_use(std::size_t first, std::size_t number) noexcept;
        void set_blocks_free(std::size_t first, std::size_t number) noexcept;

        std::size_t blockSize;
        std::size_t blockCount;

        std::size_t memorySizeBytes;
        std::size_t ledgerSizeBytes;

        std::byte* memory;
        std::byte* ledger;
    };

    MemoryBucket::MemoryBucket() noexcept
        : blockSize{ 0 }
        , blockCount{ 0 }
        , memorySizeBytes{ 0 }
        , ledgerSizeBytes{ 0 }
        , memory{ nullptr }
        , ledger{ nullptr }
    { }

    MemoryBucket::~MemoryBucket() noexcept
    { }

    void MemoryBucket::initialize(std::size_t blockSize, std::size_t blockCount, AllocatorBase* allocator) noexcept
    {
        this->blockSize = blockSize;
        this->blockCount = blockCount;

        memorySizeBytes = blockSize * blockCount;
        ledgerSizeBytes = 1 + ((blockCount - 1) / 8);

        memory = allocator->allocate(memorySizeBytes);
        ledger = allocator->allocate(ledgerSizeBytes);

        std::memset(ledger, 0, ledgerSizeBytes);
    }

    [[nodiscard]] std::byte* MemoryBucket::allocate(std::size_t memorySizeBytes) noexcept
    {
        const std::size_t blockNum = 1 + ((memorySizeBytes - 1) / blockSize);
        const std::size_t blockId = find_contiguous_blocks(blockNum);
        if (blockId == blockCount)
        {
            return nullptr;
        }
        set_blocks_in_use(blockId, blockNum);

        return memory + blockId * blockSize;
    }

    void MemoryBucket::deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept
    {
        const std::size_t blockNum = 1 + ((memorySizeBytes - 1) / blockSize);
        const std::size_t blockId = static_cast<std::size_t>(ptr - memory) / blockSize;
        set_blocks_free(blockId, blockNum);
    }

    const bool MemoryBucket::is_belongs(std::byte* ptr) const noexcept
    {
        return (ptr >= memory) && (ptr < (memory + memorySizeBytes));
    }

    const std::size_t MemoryBucket::get_block_size() const noexcept
    {
        return blockSize;
    }
    
    const std::size_t MemoryBucket::get_block_count() const noexcept
    {
        return blockCount;
    }

    const bool MemoryBucket::is_initialized() const noexcept
    {
        return blockCount != 0;
    }

    std::size_t MemoryBucket::find_contiguous_blocks(std::size_t number) const noexcept
    {
        std::size_t blockCounter = 0;   // contains the number of contiguous free blocks
        std::size_t currentBlockId = 0; // contains id of current block
        std::size_t firstBlockId = 0;   // contains id of the first block in the group of contiguous free blocks

        for (std::size_t it = 0; it < ledgerSizeBytes; it++)
        {
            std::byte ledgerByte = *(ledger + it);
            if (static_cast<uint8_t>(ledgerByte) == 255)
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
        firstBlockId = blockCount;

        block_found:

        return firstBlockId;
    }

    void MemoryBucket::set_blocks_in_use(std::size_t first, std::size_t number) noexcept
    {
        for (std::size_t it = 0; it < number; it++)
        {
            std::size_t currentByte = (first + it) / 8;
            std::size_t currentBit = 7 - (first + it) % 8; // "7 -" is used here to reverse bit order, so bit zero is the left-most bit of value
            std::byte* byte = ledger + currentByte;
            *byte = set_bit(*byte, currentBit);
        }
    }

    void MemoryBucket::set_blocks_free(std::size_t first, std::size_t number) noexcept
    {
        for (std::size_t it = 0; it < number; it++)
        {
            std::size_t currentByte = (first + it) / 8;
            std::size_t currentBit = 7 - (first + it) % 8;
            std::byte* byte = ledger + currentByte;
            *byte = remove_bit(*byte, currentBit);
        }
    }

    struct BucketDescrition
    {
        std::size_t blockSize = 0;
        std::size_t blockCount = 0;
    };

    constexpr BucketDescrition bucket_desc(std::size_t blockSizeBytes, std::size_t memorySizeBytes)
    {
        return { blockSizeBytes, memorySizeBytes / blockSizeBytes };
    }

    struct BucketCompareInfo
    {
        std::size_t bucketId;
        std::size_t blocksUsed;
        std::size_t memoryWasted;

        bool operator < (const BucketCompareInfo& other)
        {
            return (memoryWasted == other.memoryWasted) ? blocksUsed < other.blocksUsed : memoryWasted < other.memoryWasted;
        }
    };

    class PoolAllocator : public AllocatorBase
    {
    public:
        constexpr static std::size_t MAX_BUCKETS = 5;

        PoolAllocator() = default;
        ~PoolAllocator() = default;

        virtual [[nodiscard]] std::byte* allocate(std::size_t memorySizeBytes) noexcept override;
        virtual void deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept override;

        void initialize(std::array<BucketDescrition, MAX_BUCKETS> bucketDescriptions, AllocatorBase* allocator) noexcept;

    private:
        std::array<MemoryBucket, MAX_BUCKETS> buckets;
    };

    [[nodiscard]] std::byte* PoolAllocator::allocate(std::size_t memorySizeBytes) noexcept
    {
        BucketCompareInfo comapreInfos[MAX_BUCKETS];
        std::size_t it;

        for (it = 0; it < MAX_BUCKETS; it++)
        {
            MemoryBucket bucket = buckets[it];
            if (!bucket.is_initialized()) break;

            comapreInfos[it].bucketId = it;
            if (bucket.get_block_size() >= memorySizeBytes)
            {
                comapreInfos[it].blocksUsed = 1;
                comapreInfos[it].memoryWasted = bucket.get_block_size() - memorySizeBytes;
            }
            else
            {
                const std::size_t blockNum = 1 + ((memorySizeBytes - 1) / bucket.get_block_size());
                const std::size_t blockMemory = blockNum * bucket.get_block_size();

                comapreInfos[it].blocksUsed = blockNum;
                comapreInfos[it].memoryWasted = blockMemory - memorySizeBytes;
            }
        }

        // sort compareInfos from [0 to it)
        // @TODO : replace std::sort mb
        std::sort(&comapreInfos[0], &comapreInfos[it - 1]);

        for (std::size_t allocIt = 0; allocIt < it; allocIt++)
        {
            auto id = comapreInfos[allocIt].bucketId;
            auto* ptr = buckets[id].allocate(memorySizeBytes);
            if (ptr) return ptr;
        }

        return nullptr;
    }

    void PoolAllocator::deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept
    {
        for (std::size_t it = 0; it < MAX_BUCKETS; it++)
        {
            MemoryBucket bucket = buckets[it];
            if (!bucket.is_initialized()) break;

            if (bucket.is_belongs(ptr))
            {
                bucket.deallocate(ptr, memorySizeBytes);
                break;
            }
        }
    }

    void PoolAllocator::initialize(std::array<BucketDescrition, MAX_BUCKETS> bucketDescriptions, AllocatorBase* allocator) noexcept
    {
        for (std::size_t it = 0; it < MAX_BUCKETS; it++)
        {
            if (bucketDescriptions[it].blockSize == 0 || bucketDescriptions[it].blockCount == 0)
            {
                continue;
            }
            buckets[it].initialize(bucketDescriptions[it].blockSize, bucketDescriptions[it].blockCount, allocator);
        }
    }
}

#endif
