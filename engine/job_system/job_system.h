#ifndef AL_JOB_SYSTEM_H
#define AL_JOB_SYSTEM_H

#include <cstddef>  // for std::size_t
#include <span>     // for std::span

#include "job_system_job.h"
#include "job_system_thread.h"
#include "engine/config/engine_config.h"

#include "utilities/thread_safe/thread_safe_queue.h"
#include "utilities/array_container.h"
#include "utilities/static_unordered_list.h"

namespace al::engine
{
    class JobSystem
    {
    public:
        static void         construct           (std::size_t numThreads)    noexcept;
        static void         destruct            ()                          noexcept;
        static JobSystem*   get_main_system     ()                          noexcept;
        static JobSystem*   get_render_system   ()                          noexcept;
        
        Job* get_job    () noexcept;
        void return_job (Job* job) noexcept;
        void start_job  (Job* job) noexcept;

        void add_job_to_queue   (Job* job)  noexcept;
        Job* get_job_from_queue ()          noexcept;
        void wait_for           (Job* job)  noexcept;

        std::span<JobSystemThread> get_threads() noexcept;

    private:
        static JobSystem* mainSystemInstance;
        static JobSystem* renderSystemInstance;

        static Job                                                 jobs[EngineConfig::MAX_JOBS];   // Stores actual job objects
        static StaticThreadSafeQueue<Job*, EngineConfig::MAX_JOBS> freeJobs;                       // Stores unused jobs

        std::span<JobSystemThread>                          threads;
        StaticThreadSafeQueue<Job*, EngineConfig::MAX_JOBS> jobQueue; // Stores jobs that are ready for dispatch

        JobSystem(std::size_t numThreads) noexcept;
        JobSystem() noexcept;
        ~JobSystem() noexcept;
    };
}

#endif
