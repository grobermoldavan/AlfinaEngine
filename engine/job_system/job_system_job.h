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

    struct Job
    {
        using DispatchFunction  = Function<void(Job*)>; // @TODO :  change this to function pointer : void(*)(Job*);
        using CachelinePadding  = std::byte[std::hardware_destructive_interference_size];
        using NextJobs          = ArrayContainer<Job*, EngineConfig::MAX_NEXT_JOBS>;

        NextJobs                    nextJobs;
        std::atomic<std::size_t>    previousJobsNum;
        DispatchFunction            dispatchFunction;
        JobSystem*                  jobSystem;
        void*                       userData;
        CachelinePadding            padding;
    };

    void    initialize_job              (Job* job, JobSystem* jobSystem);
    void    configure                   (Job* job, Job::DispatchFunction func, void* data = nullptr);
    void    dispatch                    (Job* job);
    bool    is_finished                 (Job* job);
    bool    is_ready_for_dispatch       (Job* job);
    void    set_before                  (Job* job, Job* other);
    void    set_after                   (Job* job, Job* other);
    void    notify_previous_job_finished(Job* job);
    void    finish                      (Job* job);
}

#endif
