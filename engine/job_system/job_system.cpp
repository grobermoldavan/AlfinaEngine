
#include <cstddef> // for std::size_t

#include "job_system.h"
#include "engine/memory/memory_manager.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    JobSystem* JobSystem::mainSystemInstance{ nullptr };
    JobSystem* JobSystem::renderSystemInstance{ nullptr };

    Job JobSystem::jobs[EngineConfig::MAX_JOBS]{ };
    StaticThreadSafeQueue<Job*, EngineConfig::MAX_JOBS> JobSystem::freeJobs{ };

    JobSystem::JobSystem(std::size_t numThreads) noexcept
        : threads{ reinterpret_cast<JobSystemThread*>(MemoryManager::get_stack()->allocate(sizeof(JobSystemThread) * numThreads)), numThreads }
        , jobQueue{ }
    {
        for (JobSystemThread& thread : threads)
        {
            new(&thread) JobSystemThread{ this };
        }
    }

    JobSystem::JobSystem() noexcept
        : threads{ }
        , jobQueue{ }
    { }

    JobSystem::~JobSystem() noexcept
    {
        for (JobSystemThread& thread : threads)
        {
            thread.~JobSystemThread();
        }
    }

    void JobSystem::construct(std::size_t mainSystemNumThreads) noexcept
    {
        al_assert(!mainSystemInstance);
        al_assert(!renderSystemInstance);
        mainSystemInstance = MemoryManager::get_stack()->allocate_as<JobSystem>();
        ::new(mainSystemInstance) JobSystem{ mainSystemNumThreads };
        renderSystemInstance = MemoryManager::get_stack()->allocate_as<JobSystem>();
        ::new(renderSystemInstance) JobSystem{ };
        for (std::size_t it = 0; it < EngineConfig::MAX_JOBS; it++)
        {
            Job* job = &jobs[it];
            freeJobs.enqueue(&job);
        }
    }

    void JobSystem::destruct() noexcept
    {
        al_assert(mainSystemInstance);
        al_assert(renderSystemInstance);
        mainSystemInstance->~JobSystem();
        renderSystemInstance->~JobSystem();
    }

    JobSystem* JobSystem::get_main_system() noexcept
    {
        return mainSystemInstance;
    }

    JobSystem* JobSystem::get_render_system() noexcept
    {
        return renderSystemInstance;
    }

    Job* JobSystem::get_job() noexcept
    {
        Job* job = nullptr;
        freeJobs.dequeue(&job);
        al_assert(job);
        ::new(job) Job{ this };
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
        al_assert(job->get_job_system() == this);
        al_assert(!job->is_finished());
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
        al_assert(job->get_job_system() == this);
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
