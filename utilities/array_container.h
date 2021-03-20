#ifndef AL_ARRAY_CONTAINER_H
#define AL_ARRAY_CONTAINER_H

#include <cstddef>  // for std::size_t
#include <cstring>  // for std::memset

#include "utilities/function.h"

#define for_each_array_container(container, it) for (std::size_t it = 0; it < container.size; it++)

namespace al
{
    template<typename T, std::size_t Capacity>
    struct ArrayContainer
    {
        std::size_t size;
        T memory[Capacity];
    };

    template<typename T, std::size_t Capacity>
    T* push(ArrayContainer<T, Capacity>* array, const T& value)
    {
        if (Capacity <= array->size)
        {
            return nullptr;
        }
        array->memory[array->size] = value;
        return &array->memory[array->size++];
    }

    template<typename T, std::size_t Capacity>
    T* push(ArrayContainer<T, Capacity>* array)
    {
        if (Capacity <= array->size)
        {
            return nullptr;
        }
        return &array->memory[array->size++];
    }

    template<typename T, std::size_t Capacity>
    T* get(ArrayContainer<T, Capacity>* array, std::size_t index)
    {
        if (index >= Capacity)
        {
            return nullptr;
        }
        return &array->memory[index];
    }

    template<typename T, std::size_t Capacity>
    void remove(ArrayContainer<T, Capacity>* array, std::size_t index)
    {
        if (index >= array->size)
        {
            return;
        }
        array->memory[index] = array->memory[array->size - 1];
        std::memset(&array->memory[array->size - 1], 0, sizeof(T));
        array->size -= 1;
    }

    template<typename T, std::size_t Capacity>
    void clear(ArrayContainer<T, Capacity>* array)
    {
        std::memset(array->memory, 0, array->size * sizeof(T));
        array->size = 0;
    }

    template<typename T, std::size_t Capacity>
    std::size_t index_of(ArrayContainer<T, Capacity>* array, T* element)
    {
        return element - array->memory;
    }
}

#endif
