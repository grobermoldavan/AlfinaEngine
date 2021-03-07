#ifndef AL_DEBUG_H
#define AL_DEBUG_H

// @NOTE :  This header contains Logger class, ScopeProfiler class and assert_implementation function
//          as well as macros for calling logger, creating scope profilers and assertions.
//          Logging is done by three macros : 
//              al_log_message(format, ...)
//              al_log_warning(format, ...)
//              al_log_error(format, ...)
//              'format' and following argumets work the same way as printf format and arguments.
//          Asserting is done via al_assert(condition) macro.
//          Profiling is done via al_profile_function() and al_profile_scope(name) macros.
//              al_profile_function automatically uses function signature as a scope name
//              al_profile_scope uses user-provided name for the scope

#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <atomic>
#include <mutex>
#include <cstdio>

#include "engine/config/engine_config.h"

#ifndef __UNIQUE_NAME
#   define __PP_CAT(a, b) __PP_CAT_I(a, b)
#   define __PP_CAT_I(a, b) __PP_CAT_II(~, a ## b)
#   define __PP_CAT_II(p, res) res
#   define __UNIQUE_NAME(base) __PP_CAT(base, __LINE__)
#endif

#ifdef AL_LOGGING_ENABLED
#   define AL_LOG_FORMAT(format) "[%s] "##format##"\n"
#   define al_log_message(category, format, ...)    { using namespace al::engine; logger_printf_log(gLogger, _MESSAGE, category, AL_LOG_FORMAT(format), __VA_ARGS__); }
#   define al_log_warning(category, format, ...)    { using namespace al::engine; logger_printf_log(gLogger, _WARNING, category, AL_LOG_FORMAT(format), __VA_ARGS__); }
#   define al_log_error(category, format, ...)      { using namespace al::engine; logger_printf_log(gLogger, _ERROR  , category, AL_LOG_FORMAT(format), __VA_ARGS__); }
#else
#   define al_log_message(category, format, ...)
#   define al_log_warning(category, format, ...)
#   define al_log_error(category, format, ...)
#endif

#ifdef AL_PROFILING_ENABLED
#   define al_profile_function() ::al::engine::ScopeProfiler __UNIQUE_NAME(profiler)(__FUNCSIG__)
#   define al_profile_scope(name) ::al::engine::ScopeProfiler __UNIQUE_NAME(profiler)(name)
#else
#   define al_profile_function()
#   define al_profile_scope(name)
#endif

#ifdef _MSC_VER
#   define al_debug_break __debugbreak
#else
#   error Unsupported platform
#endif

#define al_assert(cond)                                                                     \
    if (!(cond))                                                                            \
    {                                                                                       \
        ::al::engine::assert_implementation(__FILE__, __FUNCSIG__, __LINE__, #cond); \
    }

#define al_assert_msg(cond, format, ...)                                                    \
    if (!(cond))                                                                            \
    {                                                                                       \
        al_log_error("assert", format, __VA_ARGS__);                                        \
        ::al::engine::assert_implementation(__FILE__, __FUNCSIG__, __LINE__, #cond); \
    }

#define al_crash_impl std::abort();
#define al_exception_wrap(code) try { code; } catch (const std::exception& e) { al_log_error("Exception", "%s", e.what()); al_assert(false); }
#define al_exception_wrap_no_assert(code) try { code; } catch (const std::exception& e) { al_crash_impl }

namespace al::engine
{
    extern struct Logger* gLogger;

    struct LoggerBuffer
    {
        char memory[EngineConfig::LOG_SYSTEM_BUFFER_SIZE];
        std::atomic<std::size_t> top;
    };

    void logger_buffer_clear(LoggerBuffer* buffer);
    char* logger_buffer_allocate(LoggerBuffer* buffer, std::size_t sizeBytes);

    enum LogLevel
    {
        _MESSAGE,
        _WARNING,
        _ERROR
    };

    struct Logger
    {
        uint32_t        levelFlags;
        FILE*           logOutput;
        FILE*           profileOutput;
        LoggerBuffer    logBuffer;
        LoggerBuffer    profileBuffer;
        std::mutex      logMutex;
        std::mutex      profileMutex;
    };

    void logger_construct(Logger* logger, uint32_t levelFlags = set_bit(0u, _MESSAGE) | set_bit(0u, _WARNING) | set_bit(0u, _ERROR));
    void logger_destruct(Logger* logger);

    template<typename ... Args> void logger_printf_log          (Logger* logger, LogLevel level, const char* category, const char* format, Args ... args);
    template<typename ... Args> void logger_printf_profile      (Logger* logger, const char* format, Args ... args);
                                void logger_enable_log_level    (Logger* logger, LogLevel level);
                                void logger_disable_log_level   (Logger* logger, LogLevel level);
                                void logger_flush_buffers       (Logger* logger);

    struct ScopeProfiler
    {
        using ClockType = std::chrono::steady_clock;
        using TimeType = std::invoke_result<decltype(&ClockType::now)>::type;

        const char* name;
        TimeType beginScopeTime;
        std::size_t threadId;

        ScopeProfiler(const char* name);
        ~ScopeProfiler();
    };

    void assert_implementation(const char* file, const char* function, const std::size_t line, const char* condition);
}

#endif
