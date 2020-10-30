#ifndef AL_CONCEPTS_H
#define AL_CONCEPTS_H

#include <type_traits>
#include <cstddef>

namespace al
{
    // Checks if type is POD
    template<typename T>
    concept pod = std::is_pod<T>::value;
    
    // Checks if type is a function
    template<typename T>
    concept function = std::is_function<T>::value;

    // Checks if type is not a function, but something invocable
    template<typename T, typename ... Args>
    concept invokable = !std::is_function<T>::value && std::invocable<T, Args...>;
}

#endif
