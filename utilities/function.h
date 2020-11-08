#ifndef AL_FUNCTION_H
#define AL_FUNCTION_H

#include <cstddef>
#include <type_traits>
#include <functional>

#include "concepts.h"

namespace al
{
    // @NOTE : Implementation is based on CryEngine's SmallFunction
    //         https://github.com/CRYTEK/CRYENGINE/blob/release/Code/CryEngine/CryCommon/CryCore/SmallFunction.h
    //         But this class allows user to wrap calls to a object methods by passing a pointer to an object 
    //         and class method

    template<typename Signature, std::size_t BufferSize = 64> class Function;

    template<typename ReturnType, typename ... Args, std::size_t BufferSize>
    class Function<ReturnType(Args...), BufferSize>
    {
    public:
        Function() noexcept
            : invoke{ nullptr }
            , destruct{ [](void*) { /* Empty function */ } }
            , copy{ [](const void*, void*) { /* Empty function */ } }
        { }

        template<al::function T>
        Function(const T& pof) noexcept
        {
            initialize(&pof);
        }

        template<al::invokable<Args...> T, typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type, Function>::value>::type>
        Function(const T& invokable) noexcept
        {
            initialize(invokable);
        }

        Function(const Function& other) noexcept
            : invoke{ other.invoke }
            , destruct{ other.destruct }
            , copy{ other.copy }
        {
            copy(&other.memory, &this->memory);
        }

        template<typename T, typename Func>
        Function(T* obj, const Func& func)
        {
            initialize_obj(obj, func);
        }

        template<al::function T>
        Function& operator = (const T& pof) noexcept
        {
            destruct(&memory);
            initialize(&pof);
            return *this;
        }

        template<al::invokable<Args...> T, typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type, Function>::value>::type>
        Function& operator = (const T& invokable) noexcept
        {
            destruct(&memory);
            initialize(invokable);
            return *this;
        }

        Function& operator = (const Function& other) noexcept
        {
            destruct(&memory);
            invoke = other.invoke;
            destruct = other.destruct;
            copy = other.copy;
            copy(&other.memory, &this->memory);
            return *this;
        }

        ~Function() noexcept
        {
            destruct(&memory);
        }

        ReturnType operator () (const Args&... args) noexcept
        {
            return invoke(&memory, args...);
        }

        void* get_host_object() noexcept
        {
            void** object = reinterpret_cast<void**>(&memory);
            return *object;
        }

        operator bool () const noexcept
        {
            return invoke != nullptr;
        }

    private:
        ReturnType (*invoke)(void* memory, const Args& ... args);
        void (*destruct)(void* memory);
        void (*copy)(const void* srcMemory, void* dstMemory);

        // @NOTE : First sizeof(void*) bytes of memory is used to save pointer 
        //         to a object, which method we might want to call.
        //         If object of this class is used to wrap a plain old function
        //         or some invokable struct this pointer is set to nullptr,
        //         otherwise it contains actual address of an object
        typename std::aligned_storage<BufferSize + sizeof(void*)>::type memory;

        // @NOTE : This method run initialization logic in case of plain old
        //         function or some invokable struct (or smthng like that)
        template<typename InvokableType>
        void initialize(InvokableType&& invokable) noexcept
        {
            using InvokableTypeNoCvref = typename std::remove_cvref<InvokableType>::type;

            static_assert( std::is_move_constructible<InvokableTypeNoCvref>::value );           // Has move constructor
            static_assert( noexcept(InvokableTypeNoCvref{ std::declval<InvokableType&&>() }) ); // Move constructor is noexcept
            static_assert( sizeof(InvokableTypeNoCvref) <= BufferSize );                        // Small enough to be stored in this object

            // Set object ptr to nullptr
            void** objectPtrAddr = reinterpret_cast<void**>(&memory);
            *objectPtrAddr = nullptr;

            // Construct invokable
            std::byte* invokableAddr = reinterpret_cast<std::byte*>(&memory) + sizeof(void*);
            ::new(invokableAddr) InvokableTypeNoCvref{ std::forward<InvokableType>(invokable) };

            invoke = [](void* memory, const Args& ... args) -> ReturnType
            {
                // Invoke method simply calls std::invoke with given invokable and passed arguments
                std::byte* invokableAddr = reinterpret_cast<std::byte*>(memory) + sizeof(void*);
                InvokableTypeNoCvref* invokable = reinterpret_cast<InvokableTypeNoCvref*>(invokableAddr);
                return std::invoke(*invokable, args...);
            };
            destruct = [](void* memory)
            {
                std::byte* invokableAddr = reinterpret_cast<std::byte*>(memory) + sizeof(void*);
                reinterpret_cast<InvokableTypeNoCvref*>(invokableAddr)->~InvokableTypeNoCvref();
            };
            copy = [](const void* srcMemory, void* dstMemory)
            {
                const std::byte* srcAddr = reinterpret_cast<const std::byte*>(srcMemory);
                std::byte* dstAddr = reinterpret_cast<std::byte*>(dstMemory);
                const InvokableTypeNoCvref* invokable = reinterpret_cast<const InvokableTypeNoCvref*>(srcAddr + sizeof(void*));

                ::new(dstAddr + sizeof(void*)) InvokableTypeNoCvref{ *invokable };  // First copy-construct invokable
                std::memset(dstAddr, 0, sizeof(void*));                             // Then set object ptr to nullptr
            };
        }

        // @NOTE : This method run initialization logic in case of wrapping some class method
        //         and it saves the pointer to an object, which must be the "host of invokation"
        template<typename Object, typename InvokableType>
        void initialize_obj(Object* obj, InvokableType&& invokable)
        {
            using InvokableTypeNoCvref = typename std::remove_cvref<InvokableType>::type;

            static_assert( std::is_move_constructible<InvokableTypeNoCvref>::value );           // Has move constructor
            static_assert( noexcept(InvokableTypeNoCvref{ std::declval<InvokableType&&>() }) ); // Move constructor is noexcept
            static_assert( sizeof(InvokableTypeNoCvref) <= BufferSize );                        // Small enough to be stored in this object

            // Copy object pointer
            Object** objectPtrAddr = reinterpret_cast<Object**>(&memory);
            *objectPtrAddr = obj;

            // Construct invokable
            std::byte* invokableAddr = reinterpret_cast<std::byte*>(&memory) + sizeof(void*);
            ::new(invokableAddr) InvokableTypeNoCvref{ std::forward<InvokableType>(invokable) };

            invoke = [](void* memory, const Args& ... args) -> ReturnType
            {
                // Invoke method simply calls std::invoke with given invokable, object pointer and passed arguments
                Object** object = reinterpret_cast<Object**>(memory);
                std::byte* invokableAddr = reinterpret_cast<std::byte*>(memory) + sizeof(void*);
                InvokableTypeNoCvref* invokable = reinterpret_cast<InvokableTypeNoCvref*>(invokableAddr);
                return std::invoke(*invokable, *object, args...);
            };
            destruct = [](void* memory)
            {
                std::byte* invokableAddr = reinterpret_cast<std::byte*>(memory) + sizeof(void*);
                reinterpret_cast<InvokableTypeNoCvref*>(invokableAddr)->~InvokableTypeNoCvref();
            };
            copy = [](const void* srcMemory, void* dstMemory)
            {
                const std::byte* srcAddr = reinterpret_cast<const std::byte*>(srcMemory);
                std::byte* dstAddr = reinterpret_cast<std::byte*>(dstMemory);
                const InvokableTypeNoCvref* invokable = reinterpret_cast<const InvokableTypeNoCvref*>(srcAddr + sizeof(void*));

                ::new(dstAddr + sizeof(void*)) InvokableTypeNoCvref{ *invokable };  // First copy-construct invokable
                std::memcpy(dstAddr, srcAddr, sizeof(void*));                       // Then copy object ptr
            };
        }
    };
}

#endif
