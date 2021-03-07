#ifndef AL_PROCEDURAL_WRAP_H
#define AL_PROCEDURAL_WRAP_H

namespace al
{
    template<typename T, typename ... Args>
    void construct(T* obj, Args ... args)
    {
        ::new(obj) T{ args... };
    }

    template<typename T>
    void destruct(T* obj)
    {
        obj->~T();
    }
}

#endif
