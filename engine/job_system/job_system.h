#ifndef AL_JOB_SYSTEM_H
#define AL_JOB_SYSTEM_H

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <thread>
#include <span>
#include <chrono>
#include <cstring>
#include <system_error>

#include "engine/asserts/asserts.h"
#include "engine/memory/allocator_base.h"
#include "utilities/concepts.h"
#include "utilities/non_copyable.h"
#include "utilities/thread_safe/thread_safe_queue.h"

namespace al::engine
{
    class Job : NonCopyable
    {
    public:
        using DispatchFunction = void(*)(Job*);

        Job() noexcept;
        Job(DispatchFunction func, Job* parent = nullptr) noexcept;

        template<pod T> void set_data(const T& data) noexcept;
        template<pod T> T& get_data() noexcept;

        void dispatch() noexcept;
        bool is_finished() const noexcept;

    protected:
        std::atomic<std::size_t> unfinishedJobs;
        DispatchFunction dispatchFunction;
        Job* parent;

        static constexpr std::size_t JOB_PAYLOAD_SIZE = sizeof(unfinishedJobs) + sizeof(dispatchFunction) + sizeof(parent);
        static constexpr std::size_t JOB_MAX_PADDING_SIZE = std::hardware_destructive_interference_size;
        static constexpr std::size_t JOB_PADDING_SIZE = JOB_MAX_PADDING_SIZE - JOB_PAYLOAD_SIZE;

        std::byte padding[JOB_PADDING_SIZE];

        void finish() noexcept;
    };

    class JobSystem;

    class JobSystemThread : NonCopyable
    {
    public:
        JobSystemThread(JobSystem* jobSystem) noexcept;
        ~JobSystemThread() noexcept;

    private:
        std::thread thread;
        std::atomic<bool> shouldRun;
        JobSystem* jobSystem;

        void work() noexcept;
        Job* get_job() noexcept;

        friend JobSystem;
    };

    class JobSystem : NonCopyable
    {
    public:
        JobSystem(std::size_t numThreads, AllocatorBase* allocator) noexcept;
        ~JobSystem() noexcept;

        void add_job(Job* job) noexcept;
        Job* get_job() noexcept;
        void wait_for(Job* job) noexcept;

    public:
        static constexpr std::size_t MAX_JOBS = 1024 * 4;

        std::span<JobSystemThread> threads;
        StaticThreadSafeQueue<Job*, MAX_JOBS> jobs;

        friend JobSystemThread;
    };

    Job::Job() noexcept
        : unfinishedJobs{ 1 }
        , dispatchFunction{ nullptr }
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

    template<pod T>
    void Job::set_data(const T& data) noexcept
    {
        static_assert(sizeof(data) <= JOB_PADDING_SIZE);
        std::memcpy(padding, &data, sizeof(data));
    }

    template<pod T>
    T& Job::get_data() noexcept
    {
        return *reinterpret_cast<T*>(padding);
    }

    void Job::dispatch() noexcept
    {
        al_assert(dispatchFunction);
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
                std::this_thread::sleep_for(std::chrono::milliseconds{ 15 });
            }
        }
    }

    Job* JobSystemThread::get_job() noexcept
    {
        return jobSystem->get_job();
    }

    JobSystem::JobSystem(std::size_t numThreads, AllocatorBase* allocator) noexcept
        : threads{ reinterpret_cast<JobSystemThread*>(allocator->allocate(sizeof(JobSystemThread) * numThreads)), numThreads }
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
                std::this_thread::sleep_for(std::chrono::milliseconds{ 5 });
            }
        }
    }
}

#endif
