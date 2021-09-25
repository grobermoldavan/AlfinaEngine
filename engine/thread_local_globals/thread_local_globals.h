#ifndef AL_THREAD_LOCAL_GLOBALS_H
#define AL_THREAD_LOCAL_GLOBALS_H

namespace al
{
    struct Logger;

    struct ApplicationGlobals
    {
        Logger* logger;
    };

    void thread_local_globals_register(ApplicationGlobals* globals);
    ApplicationGlobals* thread_local_globals_access();
}

#endif
