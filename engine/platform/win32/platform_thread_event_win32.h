#ifndef AL_PLATFORM_THREAD_EVENT_WIN32_H
#define AL_PLATFORM_THREAD_EVENT_WIN32_H

#include <cstdint>
#include <chrono>

#include <Windows.h>

namespace al
{
    class ThreadEvent
    {
    public:
        ThreadEvent() noexcept
            : isInvoked{ false }
            , event{ ::CreateEventA(NULL, TRUE, FALSE, NULL) }
        { }

        ~ThreadEvent() noexcept
        {
            ::CloseHandle(event);
        }

        void wait() noexcept
        {
            if (!is_invoked())
            {
                ::WaitForSingleObject(event, INFINITE);
            }
        }

        template <typename Rep, typename Period>
        bool wait_for(const std::chrono::duration<Rep, Period>& time) noexcept
        {
            if (!is_invoked())
            {
                using duration = std::chrono::duration<DWORD, std::milli>;
                using clock = std::chrono::steady_clock;

                const DWORD waitTimeMs = std::chrono::duration_cast<duration>(time).count();
                const auto startTime = clock::now().time_since_epoch();

                auto checkCondition = [this]() -> bool
                {
                    return isInvoked;
                };

                auto checkTime = [&]() -> bool
                {
                    auto currentTime = clock::now().time_since_epoch();
                    DWORD diff = std::chrono::duration_cast<duration>(currentTime - startTime).count();
                    return diff < waitTimeMs;
                };

                while (!checkCondition() && checkTime())
                {
                    auto currentTime = clock::now().time_since_epoch();
                    DWORD diff = std::chrono::duration_cast<duration>(currentTime - startTime).count();
                    if (waitTimeMs < diff)
                    {
                        break;
                    }
                    
                    ::WaitForSingleObject(event, waitTimeMs - diff);
                }
            }
            return is_invoked();
        }

        void invoke() noexcept
        {
            ::InterlockedExchange(&isInvoked, 1);
            ::SetEvent(event);
        }

        void reset() noexcept
        {
            ::InterlockedExchange(&isInvoked, 0);
            ::ResetEvent(event);
        }

        inline bool is_invoked() noexcept
        {
            return isInvoked != 0;
        }

    private:
        HANDLE event;
        volatile uint32_t isInvoked;
    };
}

#endif
