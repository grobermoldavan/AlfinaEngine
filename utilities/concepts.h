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

    template<typename T>
    concept number = std::is_floating_point<T>::value || std::is_integral<T>::value;

    template<typename T>
    concept trivially_destructible = std::is_trivially_destructible<T>::value;
}

#endif
