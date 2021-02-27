
#include "job_system_thread.h"
#include "job_system_job.h"
#include "job_system.h"

namespace al::engine
{
    JobSystemThread::JobSystemThread(JobSystem* jobSystem) noexcept
        : shouldRun{ true }
        , thread{ &JobSystemThread::work, this }
        , jobSystem{ jobSystem }
    {  }

    JobSystemThread::~JobSystemThread() noexcept
    {
        shouldRun = false;
        try
        {
            thread.join();
        }
        catch(const std::exception& e)
        {
            // @TODO : handle system error
            al_assert_msg(false, "Unable to join thread - exception was thrown. Message : %s", e.what());
        }
    }

    std::thread* JobSystemThread::get_thread() noexcept
    {
        return &thread;
    }

    void JobSystemThread::work() noexcept
    {
        while(shouldRun)
        {
            Job* job = get_job();
            if (job)
            {
                job->dispatch();
            }
            else
            {
                std::this_thread::sleep_for(EngineConfig::JOB_THREAD_SLEEP_TIME);
            }
        }
    }

    Job* JobSystemThread::get_job() noexcept
    {
        return jobSystem->get_job_from_queue();
    }
}
