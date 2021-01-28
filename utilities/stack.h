#ifndef AL_STACK_H
#define AL_STACK_H

#include <cstddef>

namespace al
{
    template<typename T>
    class Stack
    {
    public:
        Stack() noexcept
            : memory{ nullptr }
            , topElement{ 0 }
            , size{ 0 }
        { }

        Stack(T* memory, std::size_t size) noexcept
            : memory{ memory }
            , topElement{ 0 }
            , size{ size }
        { }

        ~Stack() noexcept
        { }

        void initialize(T* memory, std::size_t size) noexcept
        {
            this->memory = memory;
            this->size = size;
        }

        bool push(const T& value) noexcept
        {
            if (is_full())
            {
                return false;
            }

            memory[topElement++] = value;
            return true;
        }

        bool pop(T* value) noexcept
        {
            if (is_empty()) 
            {
                return false;
            }

            *value = memory[--topElement];
            return true;
        }

        void clear() noexcept
        {
            topElement = 0;
        }

        bool is_empty() const noexcept
        {
            return topElement == 0;
        }

        bool is_full() const noexcept
        {
            return topElement == size;
        }

        bool at(T* element, std::size_t id) noexcept
        {
            if (id >= topElement)
            {
                return false;
            }

            *element = memory[id];
            return true;
        }

    private:
        T* memory;
        std::size_t topElement;
        std::size_t size;
    };

    template<typename T, std::size_t Capacity>
    class StaticStack : public Stack<T>
    {
    public:
        StaticStack()
            : Stack<T>{ storage, Capacity }
        { }

    private:
        T storage[Capacity];
    };
}

#endif