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

    template<typename ... Args, std::size_t SubscribersNum>
    class Event<void(Args...), SubscribersNum>
    {
    public:
        using FunctionT = al::Function<void(Args...)>;

        Event() noexcept
            : subscribers{ }
        { }

        inline void invoke(const Args&... args) noexcept
        {
            subscribers.for_each([&](FunctionT* func)
            {
                (*func)(args...);
            });
        }

        inline void subscribe(const FunctionT& func) noexcept
        {
            FunctionT* freeFunction = subscribers.get();
            *freeFunction = func;
        }

        // @NOTE : currently usubscription works only by removing all
        //         subscriptions of a given object
        // @TODO : add a way to unsubscribe any function
        inline void unsubscribe(void* host) noexcept
        {
            subscribers.remove_by_condition([&](FunctionT* other) -> bool
            {
                return host == other->get_host_object();
            });
        }

        inline void operator () (const Args&... args) noexcept
        {
            invoke(args...);
        }

    private:
        SuList<FunctionT, SubscribersNum> subscribers;
    };

}

#endif
