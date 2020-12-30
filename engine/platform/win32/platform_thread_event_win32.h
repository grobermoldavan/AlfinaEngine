#ifndef AL_PLATFORM_THREAD_EVENT_WIN32_H
#define AL_PLATFORM_THREAD_EVENT_WIN32_H

#include <cstdint>

#include <Windows.h>

#include "engine/platform/platform_thread_event.h"

namespace al::engine
{
    class Win32ThreadEvent : public ThreadEvent
    {
    public:
        Win32ThreadEvent() noexcept;
        ~Win32ThreadEvent() noexcept;

        virtual void wait() noexcept override;
        virtual bool wait_for(const std::chrono::milliseconds time) noexcept override;
        virtual void invoke() noexcept override;
        virtual void reset() noexcept override;
        virtual bool is_invoked() noexcept override;

    private:
        HANDLE event;
        volatile uint32_t isInvoked;
    };
}

#endif
