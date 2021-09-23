#ifndef AL_LOGGER_H
#define AL_LOGGER_H

#include "engine/memory/allocator_bindings.h"
#include "engine/utilities/thread_safe_queue.h"
#include "engine/platform/platform.h"
#include "engine/result.h"

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
        Array<PlatformFile> outputs;
        ThreadSafeQueue<LogMessage> messageQueue;
        AllocatorBindings persistentAllocator;
        ThreadSafeQueue<LogMessage>::Cell* queueCells;
        Atomic<bool> isWritingToOuptuts;
    };

    struct LoggerCreateInfo
    {
        AllocatorBindings persistentAllocator;
        PointerWithSize<PlatformFile> outputFiles;
    };

    void logger_construct(Logger* logger, LoggerCreateInfo* createInfo);
    void logger_destroy(Logger* logger);
    bool logger_flush(Logger* logger);

    template<typename ... Args>
    Result<void> logger_log(Logger* logger, LogSeverety severety, const char* fmt, Args ... args);
}

#endif
