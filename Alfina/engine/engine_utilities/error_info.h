#ifndef __ALFINA_ERROR_INFO_H__
#define __ALFINA_ERROR_INFO_H__

namespace al::engine
{
    struct [[nodiscard]] ErrorInfo
    {
        enum class Code
        {
            ALL_FINE,
            INCORRECT_INPUT_DATA,
            ALLOCATION_ERROR
        };

        enum ErrorMessageCode
        {
            ALL_FINE,
            UNABLE_TO_ALLOCATE_MEMORY,
            NULL_PTR_PROVIDED
        };

        static constexpr const char* ERROR_MESSAGES[] =
        {
            "Everything went allright.",
            "Unable to allocate memory.",
            "Null pointer provided to a method."
        };

        Code        errorCode;
        const char* errorMessage;
        
        operator bool () const noexcept { return errorCode == Code::ALL_FINE; }
    };

    // Small wrapper-function to increase readability of code
    inline bool is_error(ErrorInfo err)
    {
        return !err;
    }
}

#endif
