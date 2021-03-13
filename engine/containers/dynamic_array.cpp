
#include "dynamic_array.h"

namespace al::engine
{
    template<typename T>
    void construct(DynamicArray<T>* array, AllocatorBase* allocator)
    {
        al_memzero(array);
        array->allocator = allocator;
    }

    template<typename T>
    void destruct(DynamicArray<T>* array)
    {
        if (array->capacity)
        {
            array->allocator->deallocate(reinterpret_cast<std::byte*>(array->memory), sizeof(T) * array->capacity);
        }
        // @TODO :  maybe need to call destructors here if neccsessary
    }

    template<typename T>
    void push(DynamicArray<T>* array, const T& value)
    {
        if (array->size == array->capacity)
        {
            expand(array, array->capacity ? array->capacity + array->capacity / 2 : 4);
        }
        array->memory[array->size++] = value;
    }

    template<typename T>
    void expand(DynamicArray<T>* array, std::size_t newCapacity)
    {
        if (array->capacity >= newCapacity)
        {
            return;
        }
        T* newMemory = reinterpret_cast<T*>(array->allocator->allocate(sizeof(T) * newCapacity));
        if (array->size)
        {
            std::memcpy(newMemory, array->memory, sizeof(T) * array->size);
        }
        if (array->capacity)
        {
            array->allocator->deallocate(reinterpret_cast<std::byte*>(array->memory), sizeof(T) * array->capacity);
        }
        array->memory = newMemory;
        array->capacity = newCapacity;
    }

    template<typename T>
    T* get(DynamicArray<T>* array, std::size_t index)
    {
        if (array->size <= index)
        {
            return nullptr;
        }
        return &array->memory[index];
    }
}
