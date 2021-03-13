
#include "array_view.h"

namespace al::engine
{
    template<typename T, std::size_t Size>
    void construct(ArrayView<T, Size>* view, T* memory)
    {
        view->memory = memory;
    }

    template<typename T, std::size_t Size>
    T* get(ArrayView<T, Size>* view, std::size_t index)
    {
        if (Size <= index)
        {
            return nullptr;
        }
        return &view->memory[index];
    }
}
