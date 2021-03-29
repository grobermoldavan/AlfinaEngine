#ifndef AL_FUNCTION_H
#define AL_FUNCTION_H

#include <type_traits>  // for std::is_function, std::enable_if, std::decay and other stuff
#include <concepts>     // for std::invocable
#include <utility>      // for std::move
#include <functional>   // for std::invoke

#include "engine/types.h"

namespace al
{
    template<typename T>                    concept function_object_pof         = std::is_function<T>::value;
    template<typename T, typename ... Args> concept function_object_invokable   = !std::is_function<T>::value && std::invocable<T, Args...>;

    // @NOTE :  Implementation is based on CryEngine's SmallFunction
    //          https://github.com/CRYTEK/CRYENGINE/blob/release/Code/CryEngine/CryCommon/CryCore/SmallFunction.h
    //          But this class allows user to wrap calls to a object methods by passing a pointer to an object 
    //          and class method

    // @NOTE :  This struct can be zero-initialized

    template<typename Signature, uSize BufferSize = 64> class Function;

    template<typename ReturnType, typename ... Args, uSize BufferSize>
    struct Function<ReturnType(Args...), BufferSize>
    {
        ReturnType  (*invoke)   (void* memory, void* host, const Args& ... args);
        void        (*destruct) (void* memory);
        void        (*copy)     (const void* srcMemory, void* dstMemory, const void* srcHostPtrLocatrion, void* dstHostPtrLocatrion);
        void*       host;
        u8          memory      [BufferSize];

        Function()
            : invoke    { [](void*, void*, Args...) { /* Empty function */ } }
            , destruct  { [](void*) { /* Empty function */ } }
            , copy      { [](const void*, void*, const void*, void*) { /* Empty function */ } }
        { }

        template<function_object_pof T>
        Function(const T& pof)
        {
            __initialize(&pof);
        }

        template<function_object_invokable<Args...> T, typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type, Function>::value>::type>
        Function(const T& invokable)
        {
            __initialize(invokable);
        }

        Function(const Function& other)
            : invoke{ other.invoke }
            , destruct{ other.destruct }
            , copy{ other.copy }
        {
            if (copy)
            {
                copy(&other.memory, &this->memory, &other.host, &this->host);
            }
        }

        template<typename T, typename Func>
        Function(T* obj, const Func& func)
        {
            __initialize_obj(obj, func);
        }

        ~Function()
        {
            if (destruct)
            {
                destruct(&memory);
            }
        }

        template<function_object_pof T>
        Function& operator = (const T& pof)
        {
            if (destruct)
            {
                destruct(&memory);
            }
            __initialize(&pof);
            return *this;
        }

        template<function_object_invokable<Args...> T, typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type, Function>::value>::type>
        Function& operator = (const T& invokable)
        {
            if (destruct)
            {
                destruct(&memory);
            }
            __initialize(invokable);
            return *this;
        }

        Function& operator = (const Function& other)
        {
            // @NOTE :  This is possible that we have null destruct function pointer
            //          if our function object was zero-initialized
            if (destruct)
            {
                destruct(&memory);
            }
            invoke = other.invoke;
            destruct = other.destruct;
            copy = other.copy;
            if (copy)
            {
                copy(&other.memory, &this->memory, &other.host, &this->host);
            }
            return *this;
        }

        ReturnType operator () (const Args&... args)
        {
            // @NOTE :  no nullptr check
            return invoke(&memory, host, args...);
        }

        operator bool () const
        {
            return invoke != nullptr;
        }

        template<typename InvokableType>
        void __initialize(InvokableType&& invokable)
        {
            using InvokableTypeNoCvref = typename std::remove_cvref<InvokableType>::type;
            static_assert( std::is_move_constructible<InvokableTypeNoCvref>::value );           // Has move constructor
            static_assert( noexcept(InvokableTypeNoCvref{ std::declval<InvokableType&&>() }) ); // Move constructor is noexcept
            static_assert( sizeof(InvokableTypeNoCvref) <= BufferSize );                        // Small enough to be stored in this object

            host = nullptr;
            ::new(memory) InvokableTypeNoCvref{ std::forward<InvokableType>(invokable) };
            invoke = [](void* memory, void* host, const Args& ... args) -> ReturnType
            {
                return std::invoke(*static_cast<InvokableTypeNoCvref*>(memory), args...);
            };
            destruct = [](void* memory)
            {
                static_cast<InvokableTypeNoCvref*>(memory)->~InvokableTypeNoCvref();
            };
            copy = [](const void* srcMemory, void* dstMemory, const void* srcHostPtrLocatrion, void* dstHostPtrLocatrion)
            {
                ::new(dstMemory) InvokableTypeNoCvref{ *static_cast<const InvokableTypeNoCvref*>(srcMemory) };
                std::memcpy(dstHostPtrLocatrion, srcHostPtrLocatrion, sizeof(void*));
            };
        }

        template<typename Object, typename InvokableType>
        void __initialize_obj(Object* obj, InvokableType&& invokable)
        {
            using InvokableTypeNoCvref = typename std::remove_cvref<InvokableType>::type;
            static_assert( std::is_move_constructible<InvokableTypeNoCvref>::value );           // Has move constructor
            static_assert( noexcept(InvokableTypeNoCvref{ std::declval<InvokableType&&>() }) ); // Move constructor is noexcept
            static_assert( sizeof(InvokableTypeNoCvref) <= BufferSize );                        // Small enough to be stored in this object

            host = obj;
            ::new(memory) InvokableTypeNoCvref{ std::forward<InvokableType>(invokable) };
            invoke = [](void* memory, void* host, const Args& ... args) -> ReturnType
            {
                Object* object = static_cast<Object*>(host);
                return std::invoke(*static_cast<InvokableTypeNoCvref*>(memory), object, args...);
            };
            destruct = [](void* memory)
            {
                static_cast<InvokableTypeNoCvref*>(memory)->~InvokableTypeNoCvref();
            };
            copy = [](const void* srcMemory, void* dstMemory, const void* srcHostPtrLocatrion, void* dstHostPtrLocatrion)
            {
                ::new(dstMemory) InvokableTypeNoCvref{ *static_cast<const InvokableTypeNoCvref*>(srcMemory) };
                std::memcpy(dstHostPtrLocatrion, srcHostPtrLocatrion, sizeof(void*));
            };
        }
    };
}

#endif
