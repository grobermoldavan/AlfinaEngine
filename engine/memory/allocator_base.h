#ifndef AL_ALLOCATOR_BASE_H
#define AL_ALLOCATOR_BASE_H

#include <cstddef> // for std::size_t

namespace al::engine
{
    struct AllocatorBindings
    {
        [[nodiscard]] void* (*allocate)(void* allocator, std::size_t memorySizeBytes);
        void (*deallocate)(void* allocator, void* ptr, std::size_t memorySizeBytes);
        void* allocator;
    };

    class AllocatorBase 
    {
    public:
        virtual [[nodiscard]] std::byte* allocate(std::size_t memorySizeBytes) noexcept = 0;
        virtual void deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept = 0;

        template<typename T>
        inline [[nodiscard]] T* allocate_as()
        {
            return reinterpret_cast<T*>(allocate(sizeof(T)));
        }

        template<typename T, typename ... Args>
        inline [[nodiscard]] T* allocate_and_construct(Args ... args)
        {
            T* instance = allocate_as<T>();
            ::new(instance) T{ args... };
            return instance;
        }

        template<typename T>
        inline void deallocate_as(T* ptr)
        {
            deallocate(reinterpret_cast<std::byte*>(ptr), sizeof(T));
        }
    };
}

#endif
