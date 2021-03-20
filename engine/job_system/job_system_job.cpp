
#include "job_system_job.h"
#include "job_system.h"

#include "engine/debug/debug.h"

#include "utilities/memory.h"
#include "utilities/procedural_wrap.h"

namespace al::engine
{
    void initialize(Job* job, JobSystem* jobSystem)
    {
        wrap_construct(&job->previousJobsNum, std::size_t{ 0 });
        job->jobSystem = jobSystem;
        job->userData = nullptr;
        al_memzero(&job->nextJobs);
    }

    void configure(Job* job, Job::DispatchFunction func, void* data)
    {
        std::atomic_store_explicit(&job->previousJobsNum, 1, std::memory_order_relaxed);
        job->dispatchFunction = func;
        job->userData = data;
    }

    void dispatch(Job* job)
    {
        al_assert(is_ready_for_dispatch(job));
        job->dispatchFunction(job);
        finish(job);
    }
    
    bool is_finished(Job* job)
    {
        return std::atomic_load_explicit(&job->previousJobsNum, std::memory_order_relaxed) == 0;
    }

    bool is_ready_for_dispatch(Job* job)
    {
        return std::atomic_load_explicit(&job->previousJobsNum, std::memory_order_relaxed) == 1;
    }

    void set_before(Job* job, Job* other)
    {
#ifdef AL_DEBUG
        for_each_array_container(job->nextJobs, it)
        {
            Job* nextJob = *get(&job->nextJobs, it);
            al_assert_msg(other != nextJob, "Job is already stored in the nextJobs array");
        }
#endif
        al_assert(!is_finished(job));
        if (is_finished(other))
        {
            return;
        }
        push(&job->nextJobs, other);
    }

    void set_after(Job* job, Job* other)
    {
        al_assert(!is_finished(job));
        if (is_finished(other))
        {
            return;
        }
        set_before(other, job);
        std::atomic_fetch_add_explicit(&job->previousJobsNum, 1, std::memory_order_relaxed);
    }

    void notify_previous_job_finished(Job* job)
    {
        al_assert(!is_finished(job));
        al_assert(!is_ready_for_dispatch(job));
        std::atomic_fetch_sub_explicit(&job->previousJobsNum, 1, std::memory_order_relaxed);
        if (is_ready_for_dispatch(job))
        {
            add_job_to_queue(job->jobSystem, job);
        }
    }

    void finish(Job* job)
    {
        al_assert(is_ready_for_dispatch(job));
        std::atomic_fetch_sub_explicit(&job->previousJobsNum, 1, std::memory_order_relaxed);
        for_each_array_container(job->nextJobs, it)
        {
            Job* nextJob = *get(&job->nextJobs, it);
            notify_previous_job_finished(nextJob);
        };
        clear(&job->nextJobs);
        return_job(job->jobSystem, job);
    }
}
