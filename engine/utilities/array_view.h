#ifndef AL_ARRAY_VIEW_H
#define AL_ARRAY_VIEW_H

#include "engine/types.h"
#include "engine/memory/memory.h"

namespace al
{
    template<typename T>
    struct ArrayView
    {
        AllocatorBindings   bindings;
        T*                  memory;
        uSize               count;
        T& operator [] (uSize index);
    };

    template<typename T>
    void av_construct(ArrayView<T>* view, AllocatorBindings* bindings, uSize size)
    {
        view->bindings = *bindings;
        view->count = size;
        view->memory = static_cast<T*>(allocate(bindings, size * sizeof(T)));
        std::memset(view->memory, 0, size * sizeof(T));
    }

    template<typename T>
    void av_destruct(ArrayView<T> view)
    {
        deallocate(&view.bindings, view.memory, view.count * sizeof(T));
    }
    
    template<typename T>
    uSize av_size(ArrayView<T> view)
    {
        return view.count * sizeof(T);
    }

    template<typename T>
    T& ArrayView<T>::operator [] (uSize index)
    {
        return memory[index];
    }
}

#endif
