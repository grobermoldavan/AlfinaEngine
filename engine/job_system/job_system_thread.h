#ifndef AL_JOB_SYSTEM_THREAD_H
#define AL_JOB_SYSTEM_THREAD_H

#include <thread>   // for std::thread
#include <atomic>   // for std::atomic

namespace al::engine
{
    class Job;
    class JobSystem;

    class JobSystemThread
    {
    public:
        JobSystemThread(JobSystem* jobSystem) noexcept;
        ~JobSystemThread() noexcept;

        std::thread* get_thread() noexcept;

    private:
        std::atomic<bool> shouldRun;
        std::thread thread;
        JobSystem* jobSystem;

        void work() noexcept;
        Job* get_job() noexcept;
    };
}

#endif
