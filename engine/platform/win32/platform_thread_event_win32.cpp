
#include "platform_thread_event_win32.h"

#include "engine/memory/memory_manager.h"

namespace al::engine
{
    [[nodiscard]] ThreadEvent* create_thread_event()
    {
        ThreadEvent* event = MemoryManager::get()->get_pool()->allocate_as<Win32ThreadEvent>();
        ::new(event) Win32ThreadEvent{ };
        return event;
    }

    void destroy_thread_event(ThreadEvent* event)
    {
        event->~ThreadEvent();
        MemoryManager::get()->get_pool()->deallocate(reinterpret_cast<std::byte*>(event), sizeof(Win32ThreadEvent));
    }

    Win32ThreadEvent::Win32ThreadEvent() noexcept
        : isInvoked{ false }
        , event{ ::CreateEventA(NULL, TRUE, FALSE, NULL) }
    { }

    Win32ThreadEvent::~Win32ThreadEvent() noexcept
    {
        ::CloseHandle(event);
    }

    void Win32ThreadEvent::wait() noexcept
    {
        if (!is_invoked())
        {
            ::WaitForSingleObject(event, INFINITE);
        }
    }

    bool Win32ThreadEvent::wait_for(const std::chrono::milliseconds time) noexcept
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

    void Win32ThreadEvent::invoke() noexcept
    {
        ::InterlockedExchange(&isInvoked, 1);
        ::SetEvent(event);
    }

    void Win32ThreadEvent::reset() noexcept
    {
        ::InterlockedExchange(&isInvoked, 0);
        ::ResetEvent(event);
    }

    bool Win32ThreadEvent::is_invoked() noexcept
    {
        return isInvoked != 0;
    }
}
