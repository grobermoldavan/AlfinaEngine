#ifndef AL_DEBUG_H
#define AL_DEBUG_H

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <atomic>
#include <thread>
#include <mutex>

#include "engine/config/engine_config.h"

#include "debug_output.h"

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

    class Logger
    {
    public:
        enum Level
        {
            _MESSAGE,
            _WARNING,
            _ERROR
        };

        Logger(uint32_t levelFlags = set_bit(0u, _MESSAGE) | set_bit(0u, _WARNING) | set_bit(0u, _ERROR)) noexcept
            : logBufferPtr{ 0 }
            , profileBufferPtr{ 0 }
            , levelFlags{ levelFlags }
        {
            if (!EngineConfig::LOG_USE_DEFAULT_OUTPUT)
            {
                override_log_output(EngineConfig::LOG_OUTPUT_FILE);
            }

            if (!EngineConfig::PROFILE_USE_DEFAULT_OUTPUT)
            {
                override_profile_output(EngineConfig::PROFILE_OUTPUT_FILE);
            }

            // Write profile header
            printf_profile("{\"otherData\": {},\"traceEvents\":[{}");
        }

        ~Logger() noexcept
        {
            print_log_buffer();
            close_log_output();

            // Write profile footer
            printf_profile("]}");

            print_profile_buffer();
            close_profile_output();
        }

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

        void print_log_buffer() noexcept
        {
            logBuffer[logBufferPtr] = 0;
            al_exception_wrap_no_assert(*GLOBAL_LOG_OUTPUT << logBuffer);
            al_exception_wrap_no_assert(GLOBAL_LOG_OUTPUT->flush());
            logBufferPtr = 0;
        }

        void print_profile_buffer() noexcept
        {
            // @NOTE :  Without a mutex profiling seems to be messed up sometimes.
            //          Don't know the reason, tbh.
            // @TODO :  Find a way to profile data without mutex.
            std::unique_lock<std::mutex> lock{ profileMutex };

            profileBuffer[profileBufferPtr] = 0;
            al_exception_wrap_no_assert(*GLOBAL_PROFILE_OUTPUT << profileBuffer);
            al_exception_wrap_no_assert(GLOBAL_PROFILE_OUTPUT->flush());
            profileBufferPtr = 0;
        }

        inline void enable_log_level(Level level) noexcept
        {
            levelFlags = set_bit(levelFlags, level);
        }

        inline void disable_log_level(Level level) noexcept
        {
            levelFlags = remove_bit(levelFlags, level);
        }

    private:
        char logBuffer[EngineConfig::LOG_BUFFER_SIZE + 1];
        std::atomic<std::size_t> logBufferPtr;

        char profileBuffer[EngineConfig::PROFILE_BUFFER_SIZE + 1];
        std::atomic<std::size_t> profileBufferPtr;

        std::mutex profileMutex;

        uint32_t levelFlags;

        char* allocate_buffer(char* buffer, std::atomic<std::size_t>* bufferPtr, std::size_t bufferSize, std::size_t sizeToAllocate) noexcept
        {
            char* result = nullptr;

            while (true)
            {
                std::size_t currentTop = *bufferPtr;
                if ((bufferSize - currentTop) < sizeToAllocate)
                {
                    break;
                }

                std::size_t newTop = currentTop + sizeToAllocate;
                const bool casResult = bufferPtr->compare_exchange_strong(currentTop, newTop);
                if (casResult)
                {
                    result = buffer + currentTop;
                    break;
                }
            }

            return result;
        }
    };

    struct ScopeProfiler
    {
        using ClockType = std::chrono::steady_clock;
        using TimeType = std::invoke_result<decltype(&ClockType::now)>::type;

        const char* name;
        TimeType beginScopeTime;
        std::size_t threadId;

        ScopeProfiler(const char* name) noexcept
            : name{ name }
            , beginScopeTime{ ClockType::now() }
            , threadId{ std::hash<std::thread::id>{}(std::this_thread::get_id()) }
        { }

        ~ScopeProfiler() noexcept
        {
            TimeType endScopeTime = ClockType::now();
            std::chrono::duration<double, std::micro> scopeTime = endScopeTime - beginScopeTime;
            std::chrono::duration<double, std::micro> beginTime = beginScopeTime.time_since_epoch();
            
            globalLogger->printf_profile(",{\"name\":\"%s\",\"cat\":\"profile\",\"ph\":\"X\",\"pid\":0,\"tid\":%d,\"ts\":%.03f,\"dur\":%.03f}", name, threadId, beginTime.count(), scopeTime.count());
        }
    };

    void assert_implementation(const char* file, const char* function, const std::size_t line, const char* condition) noexcept
    {
        // @TODO :  Currently asserts freeze program if they get fired in the render thread.
        //          This happends because after assert renderer does not fire onFrameProcessEnd event
        //          and main thread just keeps waiting for this event forever.
        al_log_error("assert", "Assertion failed. File : %s, function %s, line : %d, condition : %s", file, function, line, condition);
        globalLogger->print_log_buffer();
        globalLogger->print_profile_buffer();
        al_debug_break();
        al_crash_impl;
    }
}

#endif
