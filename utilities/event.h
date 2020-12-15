#ifndef AL_EVENT_H
#define AL_EVENT_H

#include <functional>
#include <tuple>

#include "function.h"
#include "static_unordered_list.h"

namespace al
{
    template<typename Signature, std::size_t SubscribersNum = 8> class Event;

    // @NOTE : Event class allows user to define events with a custom signature
    //         like this : Event<void(int, double)> indDoubleEvent.
    //         Any al::Function with a matching signature can be subscribed to
    //         this event and all subscribed functions will be invoked when event
    //         is fired.
    //         Function can be unsubscribed from event only via host object.

    using EventHandle = uint64_t;

    template<typename ... Args, std::size_t SubscribersNum>
    class Event<void(Args...), SubscribersNum>
    {
    public:
        using FunctionT = al::Function<void(Args...)>;

        struct EventEntry
        {
            FunctionT func;
            EventHandle handle;
        };

        Event() noexcept
            : subscribers{ }
        { }

        void invoke(const Args&... args) noexcept
        {
            subscribers.for_each([&](EventEntry* entry)
            {
                entry->func(args...);
            });
        }

        EventHandle subscribe(const FunctionT& func) noexcept
        {
            EventEntry* freeEntry = subscribers.get();
            freeEntry->func = func;
            freeEntry->handle = get_next_handle();
            return freeEntry->handle;
        }

        // @NOTE : unsubscribe by host object
        void unsubscribe(void* host) noexcept
        {
            subscribers.remove_by_condition([&](EventEntry* other) -> bool
            {
                return host == other->func.get_host_object();
            });
        }

        // @NOTE : unsubscribe by handle
        void unsubscribe(EventHandle handle) noexcept
        {
            subscribers.remove_by_condition([&](EventEntry* other) -> bool
            {
                return handle == other->handle;
            });
        }

        inline void operator () (const Args&... args) noexcept
        {
            invoke(args...);
        }

    private:
        SuList<EventEntry, SubscribersNum> subscribers;

        EventHandle get_next_handle() const noexcept
        {
            static EventHandle handle = 0;
            return handle++;
        }
    };

}

#endif
