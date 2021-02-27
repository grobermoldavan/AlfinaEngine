#ifndef AL_JOB_SYSTEM_JOB_H
#define AL_JOB_SYSTEM_JOB_H

#include <cstddef>  // for std::size_t and std::byte
#include <atomic>   // for std::atomic
#include <new>      // for std::hardware_destructive_interference_size

#include "utilities/non_copyable.h"
#include "utilities/function.h"
#include "utilities/array_container.h"

namespace al::engine
{
    class JobSystem;

    class Job : NonCopyable
    {
    public:
        using DispatchFunction = Function<void(Job*)>;

        Job() noexcept;
        Job(JobSystem* jobSystem) noexcept;
        ~Job() = default;

        void configure(DispatchFunction func, void* data = nullptr) noexcept;
        JobSystem* get_job_system() noexcept;

        void    dispatch() noexcept;
        bool    is_finished() const noexcept;
        bool    is_ready_for_dispatch() const noexcept;
        void*   get_user_data() noexcept;
        void    set_before(Job* job) noexcept;
        void    set_after(Job* job) noexcept;

        void    notify_previous_job_finished() noexcept;

    private:
        using CachelinePadding  = std::byte[std::hardware_destructive_interference_size];
        using NextJobs          = ArrayContainer<Job*, EngineConfig::MAX_NEXT_JOBS>;

        NextJobs                    nextJobs;
        std::atomic<std::size_t>    previousJobsNum;
        DispatchFunction            dispatchFunction;
        JobSystem*                  jobSystem;
        void*                       userData;
        CachelinePadding            padding;

        void finish() noexcept;
    };
}

#endif
