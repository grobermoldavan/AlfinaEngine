#ifndef AL_FUNCTION_H
#define AL_FUNCTION_H

#include <cstddef>
#include <type_traits>

#include "concepts.h"

#define AL_SAFE_CALL(func, ...) if (func) func(__VA_ARGS__)

namespace al
{
    // @NOTE : Implementation is based on CryEngine's SmallFunction
    //         https://github.com/CRYTEK/CRYENGINE/blob/release/Code/CryEngine/CryCommon/CryCore/SmallFunction.h

    template<typename Signature, std::size_t BufferSize = 64> class Function;

    template<typename ReturnType, typename ... Args, std::size_t BufferSize>
    class Function<ReturnType(Args...), BufferSize>
    {
    public:
        Function() noexcept
            : invoke{ nullptr }
            , destruct{ nullptr }
            , copy{ nullptr }
        { }

        template<al::function T>
        Function(const T& pof) noexcept
            : invoke{ nullptr }
            , destruct{ nullptr }
            , copy{ nullptr }
        {
            initialize(&pof);
        }

        template<al::invokable<Args...> T, typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type, Function>::value>::type>
        Function(const T& invokable) noexcept
            : invoke{ nullptr }
            , destruct{ nullptr }
            , copy{ nullptr }
        {
            initialize(invokable);
        }

        Function(const Function& other) noexcept
            : invoke{ other.invoke }
            , destruct{ other.destruct }
            , copy{ other.copy }
        {
            AL_SAFE_CALL(copy, &other.memory, &this->memory);
        }

        template<al::function T>
        Function& operator = (const T& pof) noexcept
        {
            AL_SAFE_CALL(destruct, &memory);
            initialize(&pof);
            return *this;
        }

        template<al::invokable<Args...> T, typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type, Function>::value>::type>
        Function& operator = (const T& invokable) noexcept
        {
            AL_SAFE_CALL(destruct, &memory);
            initialize(invokable);
            return *this;
        }

        Function& operator = (const Function& other) noexcept
        {
            AL_SAFE_CALL(destruct, &memory);
            AL_SAFE_CALL(copy, &other.memory, &this->memory);
            return *this;
        }

        ~Function() noexcept
        {
            AL_SAFE_CALL(destruct, &memory);
        }

        ReturnType operator () (const Args&... args) noexcept
        {
            return invoke(&memory, args...);
        }

        operator bool () const noexcept
        {
            return invoke != nullptr;
        }

    private:
        ReturnType (*invoke)(void* memory, const Args& ... args);
        void (*destruct)(void* memory);
        void (*copy)(const void* srcMemory, void* dstMemory);
        typename std::aligned_storage<BufferSize>::type memory;

        template<typename T>
        void initialize(T&& invokable) noexcept
        {
            using InvokableType = typename std::remove_cvref<T>::type;
            static_assert( std::is_move_constructible<InvokableType>::value );  // Has move constructor
            static_assert( noexcept(InvokableType{ std::declval<T&&>() }) );    // Move constructor is noexcept
            static_assert( sizeof(InvokableType) <= BufferSize );               // Small enough to be stored in this object
            ::new(&memory) InvokableType{ std::forward<T>(invokable) };

            invoke = [](void* memory, const Args& ... args) -> ReturnType
            {
                InvokableType* invokable = reinterpret_cast<InvokableType*>(memory);
                return (*invokable)(args...);
            };
            destruct = [](void* memory)
            {
                reinterpret_cast<InvokableType*>(memory)->~InvokableType();
            };
            copy = [](const void* srcMemory, void* dstMemory)
            {
                const InvokableType* invokable = reinterpret_cast<const InvokableType*>(srcMemory);
                ::new(dstMemory) InvokableType{ *invokable };
            };
        }
    };
}

#undef AL_SAFE_CALL

#endif
