
#include "job_system_thread.h"
#include "job_system_job.h"
#include "job_system.h"

namespace al::engine
{
    void construct(JobSystemThread* thread, JobSystem* jobSystem)
    {
        thread->shouldRun   = true;
        thread->thread      = { work, thread };
        thread->jobSystem   = jobSystem;
    }

    void destruct(JobSystemThread* thread)
    {
        thread->shouldRun = false;
        thread->thread.join();
    }

    void work(JobSystemThread* thread)
    {
        while(thread->shouldRun)
        {
            Job* job = get_job_from_queue(thread->jobSystem);
            if (job)
            {
                dispatch(job);
            }
            else
            {
                std::this_thread::sleep_for(EngineConfig::JOB_THREAD_SLEEP_TIME);
            }
        }
    }
}
