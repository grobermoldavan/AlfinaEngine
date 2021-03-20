#ifndef AL_DYNAMIC_ARRAY_H
#define AL_DYNAMIC_ARRAY_H

#include <cstddef>

#include "engine/memory/allocator_base.h"

#define for_each_dynamic_array(dynamicArray, it) for (std::size_t it = 0; it < dynamicArray.size; it++)

namespace al::engine
{
    template<typename T>
    struct DynamicArray
    {
        AllocatorBindings   bindings;
        T*                  memory;
        std::size_t         size;
        std::size_t         capacity;
    };

    template<typename T>
    void construct(DynamicArray<T>* array, AllocatorBindings bindings = get_allocator_bindings(&gMemoryManager->pool));

    template<typename T>
    void destruct(DynamicArray<T>* array);

    template<typename T>
    void push(DynamicArray<T>* array, const T& value);

    template<typename T>
    void expand(DynamicArray<T>* array, std::size_t size);

    template<typename T>
    T* get(DynamicArray<T>* array, std::size_t index);
}

#endif
