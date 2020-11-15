#ifndef AL_ASSERTS_H
#define AL_ASSERTS_H

#include <cassert>
#include <thread>

#define al_assert(cond) assert(cond)
#define al_assert_main_thread() al_assert(std::this_thread::get_id() == al::engine::GlobalAssertsData::mainThreadId)

namespace al::engine
{
    struct GlobalAssertsData
    {
        static std::thread::id mainThreadId;
    };

    std::thread::id GlobalAssertsData::mainThreadId;
}

#endif
