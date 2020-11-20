#ifndef ALFINA_SCOPE_TIMER_H
#define ALFINA_SCOPE_TIMER_H

#include <chrono>
#include <type_traits>

#include "function.h"

namespace al
{
    class ScopeTimer
    {
    public:
        using Callback = al::Function<void(double)>;
        using TimeType = std::invoke_result<decltype(&std::chrono::high_resolution_clock::now)>::type;

        ScopeTimer(const Callback& cb) noexcept
            : finishCb  { cb }
            , startTime { std::chrono::high_resolution_clock::now() }
        { }

        ~ScopeTimer() noexcept
        {
            TimeType endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> result = endTime - startTime;
            finishCb(result.count());
        }

        double get_current_time() noexcept
        {
            TimeType endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> result = endTime - startTime;
            return result.count();
        }

    private:
        Callback finishCb;
        TimeType startTime;
    };
}

#endif
