#ifndef AL_CONCEPTS_H
#define AL_CONCEPTS_H

#include <type_traits>
#include <cstddef>

namespace al
{
    template<typename T>
    concept pod = std::is_pod<T>::value;
}

#endif
