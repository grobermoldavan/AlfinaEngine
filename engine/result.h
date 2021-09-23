#ifndef AL_RESULT_H
#define AL_RESULT_H

namespace al
{
#ifdef AL_DEBUG

#define dbg(cmd) cmd
#define is_ok(arg) ::al::is_ok_impl(arg)
#define unwrap(arg) ::al::unwrap_impl(arg)

    template<typename T>
    struct Result
    {
        T value;
        const char* error;
        operator T();
    };

    template<>
    struct Result<void>
    {
        const char* error;
    };

    template<typename T>
    inline Result<T> ok(const T& value)
    {
        return { value, nullptr, };
    }

    inline Result<void> ok()
    {
        return { nullptr, };
    }

    template<typename T>
    inline Result<T> err(const char* msg)
    {
        return { {}, msg, };
    }

    template<>
    inline Result<void> err(const char* msg)
    {
        return { msg, };
    }

    template<typename T>
    inline bool is_ok_impl(const Result<T>& result)
    {
        return result.error == nullptr;
    }

    template<typename T>
    inline T unwrap_impl(const Result<T>& result)
    {
        // @TODO : assert if !is_ok
        return result.value;
    }

    template<>
    inline void unwrap_impl(const Result<void>& result)
    {
        // @TODO : assert if !is_ok
    }

    template<typename T>
    Result<T>::operator T()
    {
        return unwrap_impl(*this);
    }
    
#else // !defined(AL_DEBUG)

#define dbg(cmd)
#define is_ok(arg) true
#define unwrap(arg) arg

    template<typename T>
    using Result = T;

    template<typename T>
    inline Result<T> ok(const T& value)
    {
        return value;
    }

    inline Result<void> ok()
    {
    }

    template<typename T>
    inline Result<T> err(const char* msg)
    {
        return {};
    }

    template<>
    inline Result<void> err(const char* msg)
    {
    }

#endif // ifdef AL_DEBUG
}

#endif
