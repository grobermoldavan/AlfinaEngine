#ifndef AL_PROCEDURAL_WRAP_H
#define AL_PROCEDURAL_WRAP_H

#include "memory.h"

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
