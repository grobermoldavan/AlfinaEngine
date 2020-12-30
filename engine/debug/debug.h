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

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

// For debug output
#include <iostream>
#include <fstream>

#include "engine/config/engine_config.h"

#ifndef __UNIQUE_NAME
#   define __PP_CAT(a, b) __PP_CAT_I(a, b)
#   define __PP_CAT_I(a, b) __PP_CAT_II(~, a ## b)
#   define __PP_CAT_II(p, res) res
#   define __UNIQUE_NAME(base) __PP_CAT(base, __LINE__)
#endif

#ifdef AL_LOGGING_ENABLED
#   define AL_DBG_NAMESPACE using namespace ::al::engine::debug;
#   define AL_LOG_FORMAT(format) "[%s] "##format##"\n"
#   define al_log_message(category, format, ...)    { AL_DBG_NAMESPACE globalLogger->printf_log(Logger::Level::_MESSAGE, category, AL_LOG_FORMAT(format), __VA_ARGS__); }
#   define al_log_warning(category, format, ...)    { AL_DBG_NAMESPACE globalLogger->printf_log(Logger::Level::_WARNING, category, AL_LOG_FORMAT(format), __VA_ARGS__); }
#   define al_log_error(category, format, ...)      { AL_DBG_NAMESPACE globalLogger->printf_log(Logger::Level::_ERROR  , category, AL_LOG_FORMAT(format), __VA_ARGS__); }
#else
#   define al_log_message(format, ...)
#   define al_log_warning(format, ...)
#   define al_log_error(format, ...)
#endif

#ifdef AL_PROFILING_ENABLED
#   define al_profile_function() ::al::engine::debug::ScopeProfiler __UNIQUE_NAME(profiler)(__FUNCSIG__)
#   define al_profile_scope(name) ::al::engine::debug::ScopeProfiler __UNIQUE_NAME(profiler)(name)
#else
#   define al_profile_function()
#   define al_profile_scope(name)
#endif

#ifdef _MSC_VER
#   define al_debug_break __debugbreak
#else
#   error Unsupported platform
#endif

#define al_assert(cond) if (!(cond)) { ::al::engine::debug::assert_implementation(__FILE__, __FUNCSIG__, __LINE__, #cond); }

#define al_crash_impl { int* crash = nullptr; crash = 0; }
#define al_exception_wrap(code) try { code; } catch (const std::exception& e) { al_log_error("Exception", "%s", e.what()); al_assert(false); }
#define al_exception_wrap_no_assert(code) try { code; } catch (const std::exception& e) { al_crash_impl }

namespace al::engine::debug
{
    class Logger* globalLogger = nullptr;

    using DebugOutput = std::ostream;
    using DebugFileOutput = std::ofstream;

    DebugOutput* GLOBAL_LOG_OUTPUT{ &std::cout };
    DebugOutput* GLOBAL_PROFILE_OUTPUT{ &std::cout };

    DebugFileOutput USER_FILE_LOG_OUTPUT;
    DebugFileOutput USER_FILE_PROFILE_OUTPUT;

    void close_log_output();
    void override_log_output(const char* filename);
    void close_profile_output();
    void override_profile_output(const char* filename);

    class Logger
    {
    public:
        enum Level
        {
            _MESSAGE,
            _WARNING,
            _ERROR
        };

        Logger(uint32_t levelFlags = set_bit(0u, _MESSAGE) | set_bit(0u, _WARNING) | set_bit(0u, _ERROR)) noexcept;
        ~Logger() noexcept;

        template<typename ... Args>
        void printf_log(Level level, const char* category, const char* format, Args ... args) noexcept
        {
            // @TODO : print level with the log
            if (!is_bit_set(levelFlags, level))
            {
                return;
            }

            int size = std::snprintf(nullptr, 0, format, category, args...);
            if (size >= 0)
            {
                char* messageBuffer = allocate_buffer(logBuffer, &logBufferPtr, EngineConfig::LOG_BUFFER_SIZE, size);
                // @TODO :  handle buffer overflow (messageBuffer == nullptr)
                std::sprintf(messageBuffer, format, category, args...);
            }
            else { /* @TODO : handle negative return value */ }
        }

        template<typename ... Args>
        void printf_profile(const char* format, Args ... args) noexcept
        {
            // @NOTE :  Without a mutex profiling seems to be messed up sometimes.
            //          Don't know the reason, tbh.
            // @TODO :  Find a way to profile data without mutex.
            std::unique_lock<std::mutex> lock{ profileMutex };

            int size = std::snprintf(nullptr, 0, format, args...);
            if (size >= 0)
            {
                char* messageBuffer = allocate_buffer(profileBuffer, &profileBufferPtr, EngineConfig::PROFILE_BUFFER_SIZE, size);
                // @TODO : handle buffer overflow (messageBuffer == nullptr)
                std::sprintf(messageBuffer, format, args...);
            }
            else { /* @TODO : handle negative return value */ }
        }

        void print_log_buffer() noexcept;
        void print_profile_buffer() noexcept;

        inline void enable_log_level(Level level) noexcept;
        inline void disable_log_level(Level level) noexcept;

    private:
        char logBuffer[EngineConfig::LOG_BUFFER_SIZE + 1];
        std::atomic<std::size_t> logBufferPtr;

        char profileBuffer[EngineConfig::PROFILE_BUFFER_SIZE + 1];
        std::atomic<std::size_t> profileBufferPtr;

        std::mutex profileMutex;
        uint32_t levelFlags;

        char* allocate_buffer(char* buffer, std::atomic<std::size_t>* bufferPtr, std::size_t bufferSize, std::size_t sizeToAllocate) noexcept;
    };

    struct ScopeProfiler
    {
        using ClockType = std::chrono::steady_clock;
        using TimeType = std::invoke_result<decltype(&ClockType::now)>::type;

        const char* name;
        TimeType beginScopeTime;
        std::size_t threadId;

        ScopeProfiler(const char* name) noexcept;
        ~ScopeProfiler() noexcept;
    };

    void assert_implementation(const char* file, const char* function, const std::size_t line, const char* condition) noexcept;
}

#endif
