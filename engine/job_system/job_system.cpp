
#include "job_system.h"
#include "engine/memory/memory_manager.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    JobSystem* JobSystem::instance{ nullptr };

    JobSystem::JobSystem(std::size_t numThreads) noexcept
        : threads{ reinterpret_cast<JobSystemThread*>(MemoryManager::get_stack()->allocate(sizeof(JobSystemThread) * numThreads)), numThreads }
        , jobs{ }
        , freeJobs{ }
        , jobQueue{ }
    {
        for (JobSystemThread& thread : threads)
        {
            new(&thread) JobSystemThread{ };
        }
        for (std::size_t it = 0; it < EngineConfig::MAX_JOBS; it++)
        {
            Job* job = &jobs[it];
            freeJobs.enqueue(&job);
        }
    }

    JobSystem::~JobSystem() noexcept
    {
        for (JobSystemThread& thread : threads)
        {
            thread.~JobSystemThread();
        }
    }

    void JobSystem::construct(std::size_t numThreads) noexcept
    {
        if (instance)
        {
            return;
        }
        instance = MemoryManager::get_stack()->allocate_as<JobSystem>();
        ::new(instance) JobSystem{ numThreads };
    }

    void JobSystem::destruct() noexcept
    {
        if (!instance)
        {
            return;
        }
        instance->~JobSystem();
    }

    JobSystem* JobSystem::get() noexcept
    {
        return instance;
    }

    Job* JobSystem::get_job() noexcept
    {
        Job* job = nullptr;
        freeJobs.dequeue(&job);
        al_assert(job);
        ::new(job) Job{ };
        return job;
    }

    void JobSystem::return_job(Job* job) noexcept
    {
        al_assert(job);
        al_assert(job->is_finished());
        job->~Job();
        freeJobs.enqueue(&job);
    }

    void JobSystem::start_job(Job* job) noexcept
    {
        al_assert_msg(!job->is_finished(), "Trying to start job that is finished or not configured");
        if (job->is_ready_for_dispatch())
        {
            add_job_to_queue(job);
        }
    }

    inline std::span<JobSystemThread> JobSystem::get_threads() noexcept
    {
        return threads;
    }

    void JobSystem::add_job_to_queue(Job* job) noexcept
    {
        al_assert_msg(job->is_ready_for_dispatch(), "add_job_to_queue only adds jobs that are ready for dispatch");
        bool result = jobQueue.enqueue(&job);
        al_assert(result);
    }

    Job* JobSystem::get_job_from_queue() noexcept
    {
        Job* job = nullptr;
        bool result = jobQueue.dequeue(&job);
        al_assert_msg(!job || job->is_ready_for_dispatch(), "Job must be ready for dispatch");
        return job;
    }

    void JobSystem::wait_for(Job* job) noexcept
    {
        while(!job->is_finished())
        {
            Job* otherJob = get_job_from_queue();
            if (otherJob)
            {
                otherJob->dispatch();
            }
            else
            {
                std::this_thread::sleep_for(EngineConfig::JOB_THREAD_SLEEP_TIME);
            }
        }
    }
}
