#ifndef AL_PLATFORM_THREAD_EVENT_H
#define AL_PLATFORM_THREAD_EVENT_H

#include <chrono>

namespace al::engine
{
    class ThreadEvent
    {
    public:
        virtual ~ThreadEvent() = default;

        virtual void wait       ()                                      noexcept = 0;
        virtual bool wait_for   (const std::chrono::milliseconds time)  noexcept = 0;
        virtual void invoke     ()                                      noexcept = 0;
        virtual void reset      ()                                      noexcept = 0;
        virtual bool is_invoked ()                                      noexcept = 0;
    };

    [[nodiscard]] ThreadEvent* create_thread_event();
    void destroy_thread_event(ThreadEvent*);
}

#endif
