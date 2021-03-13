#ifndef AL_ARRAY_VIEW_H
#define AL_ARRAY_VIEW_H

#include <cstddef>

namespace al::engine
{
    template<typename T, std::size_t Size>
    struct ArrayView
    {
        T* memory;
    };

    template<typename T, std::size_t Size>
    void construct(ArrayView<T, Size>* view, T* memory);

    template<typename T, std::size_t Size>
    T* get(ArrayView<T, Size>* view, std::size_t index);
}

#endif
