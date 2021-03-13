
#include <thread>

#include "debug.h"
#include "engine/memory/memory_manager.h"

#include "utilities/procedural_wrap.h"

namespace al::engine
{
    Logger* gLogger = nullptr;

    void logger_buffer_clear(LoggerBuffer* buffer)
    {
        std::memset(buffer->memory, 0, sizeof(EngineConfig::LOG_SYSTEM_BUFFER_SIZE));
        std::atomic_store_explicit(&buffer->top, 0, std::memory_order_relaxed);
    }

    char* logger_buffer_allocate(LoggerBuffer* buffer, std::size_t sizeBytes)
    {
        std::size_t result;
        while(true)
        {
            std::size_t currentTop = std::atomic_load_explicit(&buffer->top, std::memory_order_relaxed);
            std::size_t freeMemory = EngineConfig::LOG_SYSTEM_BUFFER_SIZE - currentTop;
            // @NOTE :  "less or equal" is used here because we always must have at least one zero byte at the end of buffer
            //          so we can easily print as cstr.
            if (freeMemory <= sizeBytes)
            {
                return nullptr;
            }
            std::size_t newTop = currentTop + sizeBytes;
            bool casResult = std::atomic_compare_exchange_strong(&buffer->top, &currentTop, newTop);
            if (casResult)
            {
                result = currentTop;
                break;
            }
        }
        return buffer->memory + result;
    }

    void construct(Logger* logger, uint32_t levelFlags)
    {
        logger->levelFlags = levelFlags;
        logger->logOutput = stdout;
        logger->profileOutput = std::fopen(EngineConfig::PROFILE_OUTPUT_FILE, "w");
        logger_buffer_clear(&logger->logBuffer);
        logger_buffer_clear(&logger->profileBuffer);
        wrap_construct(&logger->logMutex);
        wrap_construct(&logger->profileMutex);
        // Write profile header
        logger_printf_profile(logger, "{\"otherData\": {},\"traceEvents\":[{}");
    }

    void destruct(Logger* logger)
    {
        // Write profile footer
        logger_printf_profile(logger, "]}");
        logger_flush_buffers(logger);
        wrap_destruct(&logger->logMutex);
        wrap_destruct(&logger->profileMutex);
    }

    template<typename ... Args>
    void logger_printf_log(Logger* logger, LogLevel level, const char* category, const char* format, Args ... args)
    {
        // @TODO : print level with the log
        if (!is_bit_set(logger->levelFlags, level))
        {
            return;
        }
        std::unique_lock<std::mutex> lock{ logger->logMutex };
        int size = std::snprintf(nullptr, 0, format, category, args...);
        char* buf = logger_buffer_allocate(&logger->logBuffer, size);
        std::sprintf(buf, format, category, args...);
    }

    template<typename ... Args>
    void logger_printf_profile(Logger* logger, const char* format, Args ... args)
    {
        std::unique_lock<std::mutex> lock{ logger->profileMutex };
        int size = std::snprintf(nullptr, 0, format, args...);
        char* buf = logger_buffer_allocate(&logger->profileBuffer, size);
        std::sprintf(buf, format, args...);
    }

    inline void logger_enable_log_level(Logger* logger, LogLevel level)
    {
        logger->levelFlags = set_bit(logger->levelFlags, level);
    }

    inline void logger_disable_log_level(Logger* logger, LogLevel level)
    {
        logger->levelFlags = remove_bit(logger->levelFlags, level);
    }

    void logger_flush_buffers(Logger* logger)
    {
        std::unique_lock<std::mutex> logLock{ logger->logMutex };
        std::unique_lock<std::mutex> profileLock{ logger->profileMutex };
        std::fprintf(logger->logOutput, "%s", logger->logBuffer.memory);
        logger_buffer_clear(&logger->logBuffer);
        std::fprintf(logger->profileOutput, "%s", logger->profileBuffer.memory);
        logger_buffer_clear(&logger->profileBuffer);
    }

    ScopeProfiler::ScopeProfiler(const char* name)
        : name{ name }
        , beginScopeTime{ ClockType::now() }
        , threadId{ std::hash<std::thread::id>{}(std::this_thread::get_id()) }
    { }

    ScopeProfiler::~ScopeProfiler()
    {
        TimeType endScopeTime = ClockType::now();
        std::chrono::duration<double, std::micro> scopeTime = endScopeTime - beginScopeTime;
        std::chrono::duration<double, std::micro> beginTime = beginScopeTime.time_since_epoch();
        logger_printf_profile(gLogger, ",{\"name\":\"%s\",\"cat\":\"profile\",\"ph\":\"X\",\"pid\":0,\"tid\":%d,\"ts\":%.03f,\"dur\":%.03f}\n", name, threadId, beginTime.count(), scopeTime.count());
    }

    void assert_implementation(const char* file, const char* function, const std::size_t line, const char* condition)
    {
        // @TODO :  Currently asserts freeze program if they get fired in the render thread.
        //          This happends because after assert renderer does not fire onFrameProcessEnd event
        //          and main thread just keeps waiting for this event forever.
        al_log_error("assert", "Assertion failed. File : %s, function %s, line : %d, condition : %s", file, function, line, condition);
        logger_flush_buffers(gLogger);
        al_debug_break();
        al_crash_impl;
    }
}
