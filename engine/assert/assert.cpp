
#include "assert.h"
#include "engine/result/result.h"
#include "engine/logger/logger.h"

#ifdef _WIN32
#   define al_debug_break __debugbreak
#else
#   error unsupported platform
#endif

namespace al
{
    template<typename ... Args>
    void assert_impl(const char* condition, const char* file, uSize line, const char* msgFmt, Args ... args)
    {
        al_log_error("Assertion failed at file %s, line %lld", file, line);
        al_log_error("Assertion condition %s", condition);
        if (msgFmt)
        {
            al_log_error(msgFmt, args...);
        }
        logger_flush(logger_access());
        al_debug_break();
    }
}
