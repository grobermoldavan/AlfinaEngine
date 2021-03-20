#ifndef AL_MEMORY_H
#define AL_MEMORY_H

#include <cstring> // for std::memset

#define al_memzero(obj)                                                     \
        std::memset(obj, 0, sizeof(std::remove_pointer_t<decltype(obj)>));

#endif
