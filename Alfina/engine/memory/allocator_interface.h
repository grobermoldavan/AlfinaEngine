#ifndef __ALFINA_ALLOCATOR_INTERFACE_H__
#define __ALFINA_ALLOCATOR_INTERFACE_H__

#include <cstdint>
#include <utility>

namespace al::engine
{
    using memory_t = uint8_t*;

    namespace allocator_interface_private
    {
        template<typename T>
        struct HasAllocate
        {
            template<typename U, uint8_t*(U::*)(size_t)> struct SFINAE {};
            template<typename U> static char Test(SFINAE<U, &U::allocateImpl>*);
            template<typename U> static int Test(...);
            static const bool value = sizeof(Test<T>(0)) == sizeof(char);
        };

        template<typename T>
        struct HasDeallocate
        {
            template<typename U, void(U::*)(memory_t)> struct SFINAE {};
            template<typename U> static char Test(SFINAE<U, &U::deallocateImpl>*);
            template<typename U> static int Test(...);
            static const bool value = sizeof(Test<T>(0)) == sizeof(char);
        };
    }

    template<typename T>
    class AllocatorInterface
    {
    public:
        AllocatorInterface(T* allocator) noexcept
            : allocatorImpl{ allocator }
        { 
            static_assert(allocator_interface_private::HasAllocate<T>::value, "Allocator must implement at least \"uint8_t* allocate(size_t)\" method");
        }

        constexpr const bool has_allocate() const noexcept
        {
            return allocator_interface_private::HasAllocate<T>::value;
        }

        constexpr const bool has_deallocate() const noexcept
        {
            return allocator_interface_private::HasDeallocate<T>::value;
        }

        inline [[nodiscard]] memory_t allocate(size_t sizeBytes) noexcept
        {
            if constexpr (allocator_interface_private::HasAllocate<T>::value) 
            {
                return allocatorImpl->allocateImpl(sizeBytes);
            }
            else
            {
                static_assert(false, "Allocation method is not provided in implementation.");
                return nullptr;
            }
        }

        inline void deallocate(memory_t ptr) noexcept
        {
            if constexpr (allocator_interface_private::HasDeallocate<T>::value) 
            {
                allocatorImpl->deallocateImpl(ptr);
            }
            else
            {
                static_assert(false, "Deallocation method is not provided in implementation.");
            }
        }

    private:
        T* allocatorImpl;
    };
}

#endif