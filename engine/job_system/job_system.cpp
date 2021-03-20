
#include <cstddef> // for std::size_t

#include "job_system.h"
#include "engine/memory/memory_manager.h"
#include "engine/debug/debug.h"

#include "utilities/procedural_wrap.h"

namespace al::engine
{
    JobSystem* gMainJobSystem = nullptr;
    JobSystem* gRenderJobSystem = nullptr;

    Job                                                 gJobs[EngineConfig::MAX_JOBS] = { };
    StaticThreadSafeQueue<Job*, EngineConfig::MAX_JOBS> gFreeJobs;

    void init_jobs()
    {
        wrap_construct(&gFreeJobs);
        for (std::size_t it = 0; it < EngineConfig::MAX_JOBS; it++)
        {
            Job* job = &gJobs[it];
            gFreeJobs.enqueue(&job);
        }
    }

    void construct(JobSystem* jobSystem, std::size_t numThreads)
    {
        // @TODO : replace std::string_view
        // @TODO : remove wrap_construct's
        if (numThreads)
        {
            JobSystemThread* memory = reinterpret_cast<JobSystemThread*>(allocate(&gMemoryManager->stack, sizeof(JobSystemThread) * numThreads));
            wrap_construct(&jobSystem->threads, memory, numThreads);
            for (JobSystemThread& thread : jobSystem->threads)
            {
                construct(&thread, jobSystem);
            }
        }
        else
        {
            wrap_construct(&jobSystem->threads);
        }
        wrap_construct(&jobSystem->jobQueue);
    }

    void destruct(JobSystem* jobSystem)
    {
        for (JobSystemThread& thread : jobSystem->threads)
        {
            destruct(&thread);
        }
        // @TODO : replace std::string_view
        wrap_destruct(&jobSystem->threads);
        wrap_destruct(&jobSystem->jobQueue);
        deallocate(&gMemoryManager->stack, reinterpret_cast<std::byte*>(jobSystem->threads.data()), sizeof(JobSystemThread) * jobSystem->threads.size());
    }

    Job* get_job(JobSystem* jobSystem)
    {
        Job* job = nullptr;
        gFreeJobs.dequeue(&job);
        al_assert(job);
        construct(job, jobSystem);
        return job;
    }

    void return_job(JobSystem* jobSystem, Job* job)
    {
        al_assert(job);
        al_assert(is_finished(job));
        gFreeJobs.enqueue(&job);
    }

    void start_job(JobSystem* jobSystem, Job* job)
    {
        al_assert(job->jobSystem == jobSystem);
        al_assert(!is_finished(job));
        if (is_ready_for_dispatch(job))
        {
            add_job_to_queue(jobSystem, job);
        }
    }

    void add_job_to_queue(JobSystem* jobSystem, Job* job)
    {
        al_assert(job->jobSystem == jobSystem);
        al_assert_msg(is_ready_for_dispatch(job), "add_job_to_queue only adds jobs that are ready for dispatch");
        bool result = jobSystem->jobQueue.enqueue(&job);
        al_assert(result);
    }

    Job* get_job_from_queue(JobSystem* jobSystem)
    {
        Job* job = nullptr;
        bool result = jobSystem->jobQueue.dequeue(&job);
        al_assert_msg(!job || is_ready_for_dispatch(job), "Job must be ready for dispatch");
        return job;
    }

    void wait_for(JobSystem* jobSystem, Job* job)
    {
        while(!is_finished(job))
        {
            Job* otherJob = get_job_from_queue(jobSystem);
            if (otherJob)
            {
                dispatch(otherJob);
            }
            else
            {
                std::this_thread::sleep_for(EngineConfig::JOB_THREAD_SLEEP_TIME);
            }
        }
    }
}
