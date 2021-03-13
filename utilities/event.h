#ifndef AL_EVENT_H
#define AL_EVENT_H

#include <functional>
#include <tuple>

#include "function.h"
#include "array_container.h"
#include "procedural_wrap.h"

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
        {
            construct(&subscribers);
            // @TODO :  remove after finishing migration to procedural code style
            for (std::size_t it = 0; it < SubscribersNum; it++)
            {
                wrap_construct(get(&subscribers, it));
            }
        }

        void invoke(const Args&... args) noexcept
        {
            for_each_array_container(subscribers, it)
            {
                EventEntry* entry = get(&subscribers, it);
                entry->func(args...);
            }
        }

        EventHandle subscribe(const FunctionT& func) noexcept
        {
            EventEntry* freeEntry = push(&subscribers);
            freeEntry->func = func;
            freeEntry->handle = get_next_handle();
            return freeEntry->handle;
        }

        // @NOTE : unsubscribe by host object
        void unsubscribe(void* host) noexcept
        {
            for_each_array_container(subscribers, it)
            {
                EventEntry* other = get(&subscribers, it);
                if (other->func.get_host_object() == host)
                {
                    remove(&subscribers, it);
                    it -= 1;
                }
            }
        }

        // @NOTE : unsubscribe by handle
        void unsubscribe(EventHandle handle) noexcept
        {
            for_each_array_container(subscribers, it)
            {
                EventEntry* other = get(&subscribers, it);
                if (other->handle == handle)
                {
                    remove(&subscribers, it);
                    break;
                }
            }
        }

        inline void operator () (const Args&... args) noexcept
        {
            invoke(args...);
        }

    private:
        ArrayContainer<EventEntry, SubscribersNum> subscribers;

        EventHandle get_next_handle() const noexcept
        {
            static EventHandle handle = 0;
            return handle++;
        }
    };

}

#endif
