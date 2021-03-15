#ifndef AL_JOB_SYSTEM_THREAD_H
#define AL_JOB_SYSTEM_THREAD_H

#include <thread>   // for std::thread
#include <atomic>   // for std::atomic

namespace al::engine
{
    struct Job;
    class JobSystem;

    struct JobSystemThread
    {
        std::atomic<bool> shouldRun;
        std::thread thread;
        JobSystem* jobSystem;
    };

    void construct  (JobSystemThread* thread, JobSystem* jobSystem);
    void destruct   (JobSystemThread* thread);
    void work       (JobSystemThread* thread);
}

#endif
