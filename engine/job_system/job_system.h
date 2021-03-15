#ifndef AL_JOB_SYSTEM_H
#define AL_JOB_SYSTEM_H

#include <cstddef>  // for std::size_t
#include <span>     // for std::span

#include "job_system_job.h"
#include "job_system_thread.h"
#include "engine/config/engine_config.h"

#include "utilities/thread_safe/thread_safe_queue.h"

namespace al::engine
{
    extern struct JobSystem* gMainJobSystem;
    extern struct JobSystem* gRenderJobSystem;

    extern Job                                                 gJobs[EngineConfig::MAX_JOBS];   // Stores actual job objects
    extern StaticThreadSafeQueue<Job*, EngineConfig::MAX_JOBS> gFreeJobs;                       // Stores unused jobs

    struct JobSystem
    {
        std::span<JobSystemThread>                          threads;
        StaticThreadSafeQueue<Job*, EngineConfig::MAX_JOBS> jobQueue; // Stores jobs that are ready for dispatch
    };

    void init_jobs();

    void construct(JobSystem* jobSystem, std::size_t numThreads);
    void destruct(JobSystem* jobSystem);

    Job* get_job            (JobSystem* jobSystem);
    void return_job         (JobSystem* jobSystem, Job* job);
    void start_job          (JobSystem* jobSystem, Job* job);
    void add_job_to_queue   (JobSystem* jobSystem, Job* job);
    Job* get_job_from_queue (JobSystem* jobSystem);
    void wait_for           (JobSystem* jobSystem, Job* job);

}

#endif
