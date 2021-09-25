
#include <cstdio>

#include "logger.h"
#include "engine/types.h"
#include "engine/memory/memory.h"
#include "engine/thread_local_globals/thread_local_globals.h"

namespace al
{
    void logger_wait_for_writes(Logger* logger)
    {
        while (platform_atomic_64_bit_load(&logger->isWritingToOuptuts, MemoryOrder::ACQUIRE))
        {
            platform_thread_yield();
        }
    }

    void logger_write_all_messages(Logger* logger)
    {
        LogMessage message;
        while (thread_safe_queue_dequeue(&logger->messageQueue, &message))
        {
            // MAX_LOG_LENGTH = longest SEVERETY_STR + FMT additional symbols + text buffer size
            constexpr uSize MAX_LOG_LENGTH = 7 + 4 + LogMessage::TEXT_BUFFER_SIZE;
            constexpr const char* SEVERETY_STR[] = { "message", "warning", "error" };
            constexpr const char* FMT = "[%s] %s\n";
            char fullFormattedLog[MAX_LOG_LENGTH] = {};
            uSize strLength = std::snprintf(fullFormattedLog, MAX_LOG_LENGTH, FMT, SEVERETY_STR[(u8)message.severety], message.text);
            for (al_iterator(it, logger->outputs))
            {
                unwrap(platform_file_write(get(it), fullFormattedLog, strLength));
            }
        }
    }

    void logger_construct(Logger* logger, LoggerCreateInfo* createInfo)
    {
        logger->persistentAllocator = createInfo->persistentAllocator;

        array_construct(&logger->outputs, &createInfo->persistentAllocator, createInfo->outputFiles.size);
        for (al_iterator(it, logger->outputs))
        {
            *get(it) = createInfo->outputFiles[to_index(it)];
        }

        logger->queueCells = allocate<ThreadSafeQueue<LogMessage>::Cell>(&createInfo->persistentAllocator, Logger::MESSAGE_QUEUE_SIZE);
        thread_safe_queue_construct(&logger->messageQueue, logger->queueCells, Logger::MESSAGE_QUEUE_SIZE);

        platform_atomic_64_bit_store(&logger->isWritingToOuptuts, false, MemoryOrder::RELAXED);
    }

    void logger_destroy(Logger* logger)
    {
        logger_write_all_messages(logger);
        for (al_iterator(it, logger->outputs))
        {
            platform_file_unload(get(it));
        }
        array_destruct(&logger->outputs);
        deallocate<ThreadSafeQueue<LogMessage>::Cell>(&logger->persistentAllocator, logger->queueCells);
    }

    bool logger_flush(Logger* logger)
    {
        bool expected = false;
        // Try to lock the writing flag
        if (platform_atomic_64_bit_cas(&logger->isWritingToOuptuts, &expected, true, MemoryOrder::ACQUIRE_RELEASE))
        {
            // If flag is locked by this thread, print all messages
            logger_write_all_messages(logger);
            platform_atomic_64_bit_store(&logger->isWritingToOuptuts, false, MemoryOrder::RELEASE);
            return true;
        }
        // return false if someone is already flushing message queue
        return false;
    }

    template<typename ... Args>
    Result<void> logger_log(Logger* logger, LogSeverety severety, const char* fmt, Args ... args)
    {
        const uSize userMessageSize = std::snprintf(nullptr, 0, fmt, args...);
        if (userMessageSize >= (LogMessage::TEXT_BUFFER_SIZE - 1))
        {
            return err<void>("Log message is too long.");
        }
        // Prepare log message
        LogMessage logMessage{};
        logMessage.severety = severety;
        std::sprintf(logMessage.text, fmt, args...);
        // Wait for all writes to finish
        logger_wait_for_writes(logger);
        // Try to enqueue new message
        while (!thread_safe_queue_enqueue(&logger->messageQueue, &logMessage))
        {
            if (!logger_flush(logger))
            {
                // If someone is already flushing message queue, wait for print to finish
                logger_wait_for_writes(logger);
            }
        }
        return ok();
    }

    Logger* logger_access()
    {
        return thread_local_globals_access()->logger;
    }
}
