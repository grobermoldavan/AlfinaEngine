#ifndef AL_THREAD_EVENT_H
#define AL_THREAD_EVENT_H

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>

#define exception_wrap(code) try { code; } catch (const std::exception& e) { int* crash = nullptr; crash = 0; } // @TODO : add exception handling

namespace al
{
    class ThreadEvent
    {
    public:
        ThreadEvent() noexcept
            : isValid{ false }
        { }

        ~ThreadEvent() = default;

        void wait() noexcept
        {
            if (!isValid)
            {
                std::unique_lock<std::mutex> lock{ eventMutex };
                exception_wrap( eventCv.wait(lock, [this]() -> bool { return isValid; }) );
            }
        }

        template <typename Rep, typename Period>
        bool wait_for(const std::chrono::duration<Rep, Period>& time) noexcept
        {
            if (!isValid)
            {
                std::unique_lock<std::mutex> lock{ eventMutex };
                exception_wrap( eventCv.wait_for(lock, time, [this]() -> bool { return isValid; }) );
            }
            return isValid;
        }

        void invoke() noexcept
        {
            isValid = true;
            exception_wrap( eventCv.notify_one() );
        }

        void reset() noexcept
        {
            isValid = false;
        }

        bool is_invoked() noexcept
        {
            return isValid;
        }

    private:
        bool isValid;
        std::mutex eventMutex;
        std::condition_variable eventCv;
    };
}

#endif
