
#include "job_system_job.h"
#include "job_system.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    Job::Job() noexcept
        : previousJobsNum{ 0 }
        , dispatchFunction{ }
        , userData{ nullptr }
    { }

    void Job::configure(DispatchFunction func, void* data) noexcept
    {
        previousJobsNum.store(1, std::memory_order_relaxed);
        dispatchFunction = func;
        userData = data;
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
        nextJobs.push(job);
    }

    void Job::set_after(Job* job) noexcept
    {
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
            JobSystem::get()->add_job_to_queue(this);
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
        JobSystem::get()->return_job(this);
    }
}
