#ifndef AL_ALLOCATIONS_TRACKER_H
#define AL_ALLOCATIONS_TRACKER_H

#include <cstring>
#include <type_traits>

#include "engine/types.h"
#include "engine/memory/memory.h"

namespace al
{
    template<uSize MaxAllocations>
    struct AllocationsTracker
    {
        struct Allocation
        {
            void* ptr;
            uSize size;
        };
        AllocatorBindings bindings;
        uSize numberOfAllocations;
        Allocation allocations[MaxAllocations];
    };

    template<uSize MaxAllocations>
    void allocations_tracker_construct(AllocationsTracker<MaxAllocations>* allocator, AllocatorBindings* bindings)
    {
        std::memset(allocator, 0, sizeof(AllocationsTracker<MaxAllocations>));
        allocator->bindings = *bindings;
    }

    template<uSize MaxAllocations>
    void allocations_tracker_destroy(AllocationsTracker<MaxAllocations>* allocator)
    {
        for (uSize it = 0; it < allocator->numberOfAllocations; it++)
        {
            deallocate(&allocator->bindings, allocator->allocations[it].ptr, allocator->allocations[it].size);
        }
    }

    template<uSize MaxAllocations, typename T = void>
    T* allocate(AllocationsTracker<MaxAllocations>* allocator, uSize amount)
    {
        using Allocation = typename AllocationsTracker<MaxAllocations>::Allocation;
        T* result = allocate<T>(&allocator->bindings, amount);
        Allocation allocation{};
        allocation.ptr = result;
        if constexpr (std::is_same_v<T, void>)
        {
            allocation.size = amount;
        }
        else
        {
            allocation.size = sizeof(T) * amount;
        }
        allocator->allocations[allocator->numberOfAllocations++] = allocation;
        return result;
    }

    template<uSize MaxAllocations, typename T = void>
    T* reallocate(AllocationsTracker<MaxAllocations>* allocator, T* old, uSize newAmount)
    {
        T* result = nullptr;
        for (uSize it = 0; it < allocator->numberOfAllocations; it++)
        {
            if (allocator->allocations[it].ptr == old)
            {
                deallocate(&allocator->bindings, allocator->allocations[it].ptr, allocator->allocations[it].size);
                result = allocate<T>(&allocator->bindings, newAmount);
                allocator->allocations[it].ptr = result;
                if constexpr (std::is_same_v<T, void>)
                {
                    allocator->allocations[it].size = newAmount;
                }
                else
                {
                    allocator->allocations[it].size = sizeof(T) * newAmount;
                }
                break;
            }
        }
        return result;
    }

    template<uSize MaxAllocations>
    void deallocate(AllocationsTracker<MaxAllocations>* allocator, void* ptr)
    {
        for (uSize it = 0; it < allocator->numberOfAllocations; it++)
        {
            if (allocator->allocations[it].ptr == ptr)
            {
                deallocate(&allocator->bindings, allocator->allocations[it].ptr, allocator->allocations[it].size);
                allocator->allocations[it] = allocator->allocations[allocator->numberOfAllocations - 1];
                allocator->numberOfAllocations -= 1;
                break;
            }
        }
    }
}

#endif
