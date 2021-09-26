
#include "result.h"
#include "assert.h"

namespace al
{
#ifdef AL_DEBUG

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
        al_assert_msg(is_ok(result), result.error);
        return result.value;
    }

    template<>
    inline void unwrap_impl(const Result<void>& result)
    {
        al_assert_msg(is_ok(result), result.error);
    }

    template<typename T>
    Result<T>::operator T()
    {
        return unwrap_impl(*this);
    }
    
#else // !defined(AL_DEBUG)

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

