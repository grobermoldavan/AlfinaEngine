
#include "job_system.h"

namespace al::engine
{
    Job::Job() noexcept
        : unfinishedJobs{ 0 }
        , dispatchFunction{ }
        , parent{ nullptr }
    { }

    Job::Job(DispatchFunction func, Job* parent) noexcept
        : unfinishedJobs{ 1 }
        , dispatchFunction{ func }
        , parent{ parent }
    {
        if (parent)
        {
            parent->unfinishedJobs.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void Job::dispatch() noexcept
    {
        dispatchFunction(this);
        finish();
    }
    
    bool Job::is_finished() const noexcept
    {
        return unfinishedJobs.load(std::memory_order_relaxed) == 0;
    }

    void Job::finish() noexcept
    {
        al_assert(unfinishedJobs != 0);
        unfinishedJobs.fetch_sub(1, std::memory_order_relaxed);
        if (is_finished() && parent)
        {
            parent->finish();
        }
    }

    JobSystemThread::JobSystemThread(JobSystem* jobSystem) noexcept
        : shouldRun{ true }
        , jobSystem{ jobSystem }
    { 
        thread = std::thread{ &JobSystemThread::work, this };
    }

    JobSystemThread::~JobSystemThread() noexcept
    {
        shouldRun = false;
        try
        {
            thread.join();
        }
        catch(const std::system_error& e)
        {
            // @TODO : handle system error
            al_assert(false);
        }
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
        return jobSystem->get_job();
    }

    JobSystem::JobSystem(std::size_t numThreads) noexcept
        : threads{ reinterpret_cast<JobSystemThread*>(MemoryManager::get()->get_stack()->allocate(sizeof(JobSystemThread) * numThreads)), numThreads }
        , jobs{ }
    {
        for (JobSystemThread& thread : threads)
        {
            new(&thread) JobSystemThread{ this };
        }
    }

    JobSystem::~JobSystem() noexcept
    {
        for (JobSystemThread& thread : threads)
        {
            thread.~JobSystemThread();
        }
    }

    void JobSystem::add_job(Job* job) noexcept
    {
        bool result = jobs.enqueue(&job);
        al_assert(result);
    }

    Job* JobSystem::get_job() noexcept
    {
        Job* job = nullptr;
        bool result = jobs.dequeue(&job);
        return job;
    }

    void JobSystem::wait_for(Job* job) noexcept
    {
        while(!job->is_finished())
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
}
