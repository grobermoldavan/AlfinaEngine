
#include "debug.h"

#include "engine/memory/memory_manager.h"

namespace al::engine::debug
{
    void close_log_output()
    {
        try
        {
            if (USER_FILE_LOG_OUTPUT.is_open())
            {
                USER_FILE_LOG_OUTPUT.flush();
                USER_FILE_LOG_OUTPUT.close();
            }
        }
        catch(const std::exception& e) { /* @TODO : handle exception */ }
    }

    void override_log_output(const char* filename)
    {
        close_log_output();
        try
        {
            USER_FILE_LOG_OUTPUT.open(filename, std::ofstream::out | std::ofstream::trunc);
        }
        catch(const std::exception& e) { /* @TODO : handle exception */ }
        GLOBAL_LOG_OUTPUT = &USER_FILE_LOG_OUTPUT;
    }

    void close_profile_output()
    {
        try
        {
            if (USER_FILE_PROFILE_OUTPUT.is_open())
            {
                USER_FILE_PROFILE_OUTPUT.flush();
                USER_FILE_PROFILE_OUTPUT.close();
            }
        }
        catch(const std::exception& e) { /* @TODO : handle exception */ }
    }

    void override_profile_output(const char* filename)
    {
        close_profile_output();
        try
        {
            USER_FILE_PROFILE_OUTPUT.open(filename, std::ofstream::out | std::ofstream::trunc);
        }
        catch(const std::exception& e) { /* @TODO : handle exception */ }
        GLOBAL_PROFILE_OUTPUT = &USER_FILE_PROFILE_OUTPUT;
    }

    Logger* Logger::instance{ nullptr };

    Logger::Logger(uint32_t levelFlags) noexcept
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

    Logger::~Logger() noexcept
    {
        print_log_buffer();
        close_log_output();

        // Write profile footer
        printf_profile("]}");

        print_profile_buffer();
        close_profile_output();
    }

    void Logger::construct() noexcept
    {
        if (instance)
        {
            return;
        }
        instance = MemoryManager::get_stack()->allocate_as<debug::Logger>();
        ::new(instance) debug::Logger{ };
    }

    void Logger::destruct() noexcept
    {
        if (!instance)
        {
            return;
        }
        instance->~Logger();
    }

    template<typename ... Args>
    inline void Logger::printf_log(Level level, const char* category, const char* format, Args ... args) noexcept
    {
        instance->instance_printf_log(level, category, format, args...);
    }

    template<typename ... Args>
    inline void Logger::printf_profile(const char* format, Args ... args) noexcept
    {
        instance->instance_printf_profile(format, args...);
    }

    inline void Logger::print_log_buffer() noexcept
    {
        instance->instance_print_log_buffer();
    }

    inline void Logger::print_profile_buffer() noexcept
    {
        instance->instance_print_profile_buffer();
    }
    
    inline void Logger::enable_log_level(Level level) noexcept
    {
        instance->instance_enable_log_level(level);
    }

    inline void Logger::disable_log_level(Level level) noexcept
    {
        instance->instance_disable_log_level(level);
    }

    template<typename ... Args>
    void Logger::instance_printf_log(Level level, const char* category, const char* format, Args ... args) noexcept
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
            if (!messageBuffer)
            {
                print_log_buffer();
                messageBuffer = allocate_buffer(logBuffer, &logBufferPtr, EngineConfig::LOG_BUFFER_SIZE, size);
            }
            std::sprintf(messageBuffer, format, category, args...);
        }
        else { /* @TODO : handle negative return value */ }
    }

    template<typename ... Args>
    void Logger::instance_printf_profile(const char* format, Args ... args) noexcept
    {
        // @NOTE :  Without a mutex profiling seems to be messed up sometimes.
        //          Don't know the reason, tbh.
        // @TODO :  Find a way to profile data without mutex.
        std::unique_lock<std::mutex> lock{ profileMutex };
        int size = std::snprintf(nullptr, 0, format, args...);
        if (size >= 0)
        {
            char* messageBuffer = allocate_buffer(profileBuffer, &profileBufferPtr, EngineConfig::PROFILE_BUFFER_SIZE, size);
            if (!messageBuffer)
            {
                print_profile_buffer();
                messageBuffer = allocate_buffer(logBuffer, &logBufferPtr, EngineConfig::LOG_BUFFER_SIZE, size);
            }
            std::sprintf(messageBuffer, format, args...);
        }
        else { /* @TODO : handle negative return value */ }
    }

    void Logger::instance_print_log_buffer() noexcept
    {
        logBuffer[logBufferPtr] = 0;
        al_exception_wrap_no_assert(*GLOBAL_LOG_OUTPUT << logBuffer);
        al_exception_wrap_no_assert(GLOBAL_LOG_OUTPUT->flush());
        logBufferPtr = 0;
    }

    void Logger::instance_print_profile_buffer() noexcept
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

    inline void Logger::instance_enable_log_level(Level level) noexcept
    {
        levelFlags = set_bit(levelFlags, level);
    }

    inline void Logger::instance_disable_log_level(Level level) noexcept
    {
        levelFlags = remove_bit(levelFlags, level);
    }

    char* Logger::allocate_buffer(char* buffer, std::atomic<std::size_t>* bufferPtr, std::size_t bufferSize, std::size_t sizeToAllocate) noexcept
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

    ScopeProfiler::ScopeProfiler(const char* name) noexcept
        : name{ name }
        , beginScopeTime{ ClockType::now() }
        , threadId{ std::hash<std::thread::id>{}(std::this_thread::get_id()) }
    { }

    ScopeProfiler::~ScopeProfiler() noexcept
    {
        TimeType endScopeTime = ClockType::now();
        std::chrono::duration<double, std::micro> scopeTime = endScopeTime - beginScopeTime;
        std::chrono::duration<double, std::micro> beginTime = beginScopeTime.time_since_epoch();

        Logger::printf_profile(",{\"name\":\"%s\",\"cat\":\"profile\",\"ph\":\"X\",\"pid\":0,\"tid\":%d,\"ts\":%.03f,\"dur\":%.03f}", name, threadId, beginTime.count(), scopeTime.count());
    }

    void assert_implementation(const char* file, const char* function, const std::size_t line, const char* condition) noexcept
    {
        // @TODO :  Currently asserts freeze program if they get fired in the render thread.
        //          This happends because after assert renderer does not fire onFrameProcessEnd event
        //          and main thread just keeps waiting for this event forever.
        al_log_error("assert", "Assertion failed. File : %s, function %s, line : %d, condition : %s", file, function, line, condition);
        Logger::print_log_buffer();
        Logger::print_profile_buffer();
        al_debug_break();
        al_crash_impl;
    }
}
