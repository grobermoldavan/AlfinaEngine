
#include "job_system_job.h"
#include "job_system.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    Job::Job() noexcept
        : previousJobsNum{ 0 }
        , dispatchFunction{ }
        , jobSystem{ nullptr }
        , userData{ nullptr }
    { }

    Job::Job(JobSystem* jobSystem) noexcept
        : previousJobsNum{ 0 }
        , dispatchFunction{ }
        , jobSystem{ jobSystem }
        , userData{ nullptr }
    { }

    void Job::configure(DispatchFunction func, void* data) noexcept
    {
        previousJobsNum.store(1, std::memory_order_relaxed);
        dispatchFunction = func;
        userData = data;
    }

    JobSystem* Job::get_job_system() noexcept
    {
        return jobSystem;
    }

    void Job::dispatch() noexcept
    {
        al_assert(is_ready_for_dispatch());
        dispatchFunction(this);
        finish();
    }
    
    bool Job::is_finished() const noexcept
    {
        return previousJobsNum.load(std::memory_order_relaxed) == 0;
    }

    bool Job::is_ready_for_dispatch() const noexcept
    {
        return previousJobsNum.load(std::memory_order_relaxed) == 1;
    }

    void* Job::get_user_data() noexcept
    {
        return userData;
    }

    void Job::set_before(Job* job) noexcept
    {
#ifdef AL_DEBUG
        nextJobs.for_each([job](Job** other)
        {
            al_assert_msg(job != *other, "Job is already stored in the nextJobs array");
        });
#endif
        al_assert(!is_finished());
        if (job->is_finished())
        {
            return;
        }
        nextJobs.push(job);
    }

    void Job::set_after(Job* job) noexcept
    {
        al_assert(!is_finished());
        if (job->is_finished())
        {
            return;
        }
        job->set_before(this);
        previousJobsNum.fetch_add(1, std::memory_order_relaxed);
    }

    void Job::notify_previous_job_finished() noexcept
    {
        al_assert(!is_finished());
        al_assert(!is_ready_for_dispatch());
        previousJobsNum.fetch_sub(1, std::memory_order_relaxed);
        if (is_ready_for_dispatch())
        {
            jobSystem->add_job_to_queue(this);
        }
    }

    void Job::finish() noexcept
    {
        al_assert(is_ready_for_dispatch());
        previousJobsNum.fetch_sub(1, std::memory_order_relaxed);
        nextJobs.for_each([](Job** _job)
        {
            Job* job = *_job;
            job->notify_previous_job_finished();
        });
        nextJobs.clear();
        jobSystem->return_job(this);
    }
}
