#ifndef AL_ARRAY_CONTAINER_H
#define AL_ARRAY_CONTAINER_H

#include <cstddef>
#include <span>

#include "utilities/function.h"

namespace al
{
    // @NOTE :  ArrayContainer is a memory structure which provides functionality
    //          similar to SuList - it allows to store objects in an unordered way
    //          (means the order could, and most likely will, change), iterate
    //          through an objects in a cache-friendly way (more cache friendly,
    //          than SuList) and remove objects based on some condition. In every
    //          aspect this container is faster than SuList, but it copies elements
    //          from one place to another, so it is can't be used to store non-copiable objects.
    //          Insertion time-complexity is O(1), removing is also O(1).

    template<typename T, std::size_t Size>
    class ArrayContainer
    {
    public:
        ArrayContainer() noexcept
            : currentSize{ 0 }
            , memory{ }
        { }

        ArrayContainer(std::initializer_list<T> args) noexcept
            : ArrayContainer{ }
        {
            for (const T& arg : args)
            {
                push(arg);
            }
        }

        bool push(const T& element) noexcept
        {
            if (currentSize >= Size)
            {
                return false;
            }

            at(currentSize) = element;
            currentSize += 1;
            return true;
        }

        T* get() noexcept
        {
            if (currentSize >= Size)
            {
                return nullptr;
            }

            currentSize += 1;
            return &at(currentSize - 1);
        }

        bool remove(std::size_t index) noexcept
        {
            if (index >= currentSize)
            {
                return false;
            }

            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                at(index).~T();
            }

            if (index != (currentSize - 1))
            {
                at(index) = at(currentSize - 1);
            }

            currentSize -= 1;
            return true;
        }

        void remove_by_condition(al::Function<bool(T*)> isConditionSatisfied) noexcept
        {
            for (std::size_t it = 0; it < currentSize; it++)
            {
                if (isConditionSatisfied(&at(it)))
                {
                    remove(it);
                    it -= 1;
                }
            }
        }

        void for_each(al::Function<void(T*)> userFunction) noexcept
        {
            for (std::size_t it = 0; it < currentSize; it++)
            {
                userFunction(&at(it));
            }
        }

        void for_each_interruptible(al::Function<bool(T*)> userFunction) noexcept
        {
            for (std::size_t it = 0; it < currentSize; it++)
            {
                if (!userFunction(&at(it)))
                {
                    break;
                }
            }
        }

        std::size_t get_current_size() const noexcept
        {
            return currentSize;
        }

        T& operator [] (std::size_t index) noexcept
        {
            return at(index);
        }

        T* data() noexcept
        {
            return memory;
        }

        T* begin() noexcept
        {
            return memory;
        }

        T* end() noexcept
        {
            return &memory[currentSize];
        }

    private:
        std::size_t currentSize;
        T memory[Size];

        inline T& at(std::size_t index) noexcept
        {
            return memory[index];
        }
    };
}

#endif
