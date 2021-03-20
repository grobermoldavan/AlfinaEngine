#ifndef AL_POOL_ALLOCATOR_STD_WRAP_H
#define AL_POOL_ALLOCATOR_STD_WRAP_H

#include "engine/memory/memory_manager.h"

namespace al::engine
{
    // ==========================================================================================================================================
    // @NOTE : This is a wrapper over pool allocator which gives us an ability to use custom allocator with std containers
    // ==========================================================================================================================================

    template<typename T>
    class PoolAllocatorStdWrap
    {
    public:
        using value_type = T;

        PoolAllocatorStdWrap() noexcept
            : allocator { &gMemoryManager->stack }
        { }

        template<typename U>
        PoolAllocatorStdWrap(const PoolAllocatorStdWrap<U>&) noexcept
            : allocator { &gMemoryManager->stack }
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
