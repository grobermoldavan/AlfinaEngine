#ifndef __ALFINA_ERROR_INFO_H__
#define __ALFINA_ERROR_INFO_H__

namespace al::engine
{
    struct ErrorInfo
    {
        enum Code : uint8_t
        {
            ALL_FINE,
            INCORRECT_INPUT_DATA,
            BAD_ALLOC
        };
        
        Code error_code;
        
        operator bool () { return error_code == Code::ALL_FINE; }
    };
}

#endif
