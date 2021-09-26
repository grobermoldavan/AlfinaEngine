#ifndef AL_LOGGER_H
#define AL_LOGGER_H

#include "result.h"
#include "engine/utilities/thread_safe_queue.h"
#include "engine/platform/platform.h"

// @NOTE : can't use "unwrap" macro here for some reason
#ifdef AL_DEBUG
#   define al_log_message(fmt, ...) ::al::unwrap_impl(::al::logger_log(::al::logger_access(), ::al::LogSeverety::_MESSAGE, fmt, __VA_ARGS__))
#   define al_log_warning(fmt, ...) ::al::unwrap_impl(::al::logger_log(::al::logger_access(), ::al::LogSeverety::_WARNING, fmt, __VA_ARGS__))
#   define al_log_error(fmt, ...)   ::al::unwrap_impl(::al::logger_log(::al::logger_access(), ::al::LogSeverety::_ERROR  , fmt, __VA_ARGS__))
#else
#   define al_log_message(fmt, ...) ::al::logger_log(::al::logger_access(), ::al::LogSeverety::_MESSAGE, fmt, __VA_ARGS__)
#   define al_log_warning(fmt, ...) ::al::logger_log(::al::logger_access(), ::al::LogSeverety::_WARNING, fmt, __VA_ARGS__)
#   define al_log_error(fmt, ...)   ::al::logger_log(::al::logger_access(), ::al::LogSeverety::_ERROR  , fmt, __VA_ARGS__)
#endif

namespace al
{
    struct AllocatorBindings;

    enum struct LogSeverety : u8
    {
        _MESSAGE = 0,
        _WARNING = 1,
        _ERROR   = 2,
    };

    struct LogMessage
    {
        constexpr static uSize TEXT_BUFFER_SIZE = 127;
        LogSeverety severety;
        char text[TEXT_BUFFER_SIZE];
    };

    struct Logger
    {
        static constexpr uSize MESSAGE_QUEUE_SIZE = 2048;
        static constexpr uSize MAX_OUTPUTS_NUM = 8;

        PlatformFile outputs[MAX_OUTPUTS_NUM];
        ThreadSafeQueue<LogMessage>::Cell queueCells[MESSAGE_QUEUE_SIZE];
        ThreadSafeQueue<LogMessage> messageQueue;
        Atomic<bool> isWritingToOuptuts;
    };

    struct LoggerCreateInfo
    {
        PlatformFile outputs[Logger::MAX_OUTPUTS_NUM];
    };

    void logger_construct(Logger* logger, LoggerCreateInfo* createInfo);
    void logger_destroy(Logger* logger);
    bool logger_flush(Logger* logger);

    template<typename ... Args>
    Result<void> logger_log(Logger* logger, LogSeverety severety, const char* fmt, Args ... args);

    Logger* logger_access();
}

#endif
