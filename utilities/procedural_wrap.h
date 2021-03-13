#ifndef AL_PROCEDURAL_WRAP_H
#define AL_PROCEDURAL_WRAP_H

#include <cstring>      // for std::memset
#include <type_traits>  // for std::remove_pointer

#define al_memzero(obj)                                                     \
    if constexpr (std::is_pointer_v<decltype(obj)>)                         \
    {                                                                       \
        std::memset(obj, 0, sizeof(std::remove_pointer_t<decltype(obj)>));  \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        std::memset(&obj, 0, sizeof(decltype(obj)));                        \
    }

namespace al
{
    template<typename T, typename ... Args>
    void wrap_construct(T* obj, Args ... args)
    {
        ::new(obj) T{ args... };
    }

    template<typename T>
    void wrap_destruct(T* obj)
    {
        obj->~T();
    }
}

#endif
