#ifndef AL_DYNAMIC_ARRAY_H
#define AL_DYNAMIC_ARRAY_H

#include <cstddef>
#include <type_traits>
#include <cstring>

#include "engine/memory/allocator_base.h"

namespace al::engine
{
    template<typename T>
    class DynamicArray
    {
    public:
        DynamicArray(std::size_t defaultCapacity = 4) noexcept
            : allocator{ MemoryManager::get()->get_pool() }
            , memory{ nullptr }
            , capacity{ 0 }
            , size{ 0 }
        {
            reallocate(defaultCapacity);
        }

        ~DynamicArray() noexcept
        {
            destruct_current_objects();
            allocator->deallocate(reinterpret_cast<std::byte*>(memory), capacity * sizeof(T));
        }

        T& at(std::size_t index) noexcept
        {
            return memory[index];
        }

        bool at(T* target, std::size_t index) noexcept
        {
            if (index >= size)
            {
                return false;
            }
            target = &memory[index];
            return true;
        }

        T* data() noexcept
        {
            return memory;
        }

        bool is_empty() const noexcept
        {
            return size == 0;
        }

        std::size_t current_size() const noexcept
        {
            return size;
        }

        std::size_t current_capacity() const noexcept
        {
            return capacity;
        }

        bool reserve(std::size_t newCapacity) noexcept
        {
            return reallocate(newCapacity);
        }

        T& push_back(const T& value) noexcept
        {
            if (size >= capacity)
            {
                reallocate(capacity + capacity / 2);
            }

            memory[size++] = value;
            return memory[size - 1];
        }

        void push_back(T&& value) noexcept
        {
            if (size >= capacity)
            {
                reallocate(capacity + capacity / 2);
            }

            memory[size++] = std::move(value);
        }

        T& operator [] (std::size_t index)
        {
            return at(index);
        }

        const T* begin() const noexcept
        {
            return &memory[0];
        }

        const T* end() const noexcept
        {
            return &memory[size];
        }

        T* begin() noexcept
        {
            return &memory[0];
        }

        T* end() noexcept
        {
            return &memory[size];
        }

    private:
        AllocatorBase* allocator;
        T* memory;
        std::size_t capacity;
        std::size_t size;

        bool reallocate(std::size_t newCapacity) noexcept
        {
            if (newCapacity <= capacity)
            {
                return true;
            }

            // Allocate new memory
            T* newMemory = reinterpret_cast<T*>(allocator->allocate(newCapacity * sizeof(T)));
            if (!newMemory)
            {
                return false;
            }

            // Copy data
            if constexpr (std::is_trivially_copyable_v<T>)
            {
                std::memcpy(newMemory, memory, size * sizeof(T));
            }
            else
            {
                for (std::size_t it = 0; it < size; it++)
                {
                    newMemory[it] = std::move(memory[it]);
                }
            }

            destruct_current_objects();
            allocator->deallocate(reinterpret_cast<std::byte*>(memory), capacity * sizeof(T));
            memory = newMemory;
            capacity = newCapacity;

            return true;
        }

        void destruct_current_objects() noexcept
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (std::size_t it = 0; it < size; it++)
                {
                    memory[it].~T();
                }
            }
        }
    };
}

#endif
