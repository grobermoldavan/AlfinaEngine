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

#include "engine/config/engine_config.h"

#include "engine/memory/memory_manager.h"
#include "engine/debug/debug.h"

#include "utilities/concepts.h"
#include "utilities/non_copyable.h"
#include "utilities/thread_safe/thread_safe_queue.h"
#include "utilities/function.h"

namespace al::engine
{
    class Job : NonCopyable
    {
    public:
        using DispatchFunction = Function<void(Job*)>;

        Job() noexcept;
        Job(DispatchFunction func, Job* parent = nullptr) noexcept;

        void dispatch() noexcept;
        bool is_finished() const noexcept;

    protected:
        typedef std::byte CachelinePadding[std::hardware_destructive_interference_size];

        std::atomic<std::size_t> unfinishedJobs;
        DispatchFunction dispatchFunction;
        Job* parent;

        CachelinePadding pad;

        void finish() noexcept;
    };

    class JobSystem;

    class JobSystemThread : NonCopyable
    {
    public:
        JobSystemThread() noexcept;
        ~JobSystemThread() noexcept;

        std::thread* get_thread() noexcept;

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
        static void construct   (std::size_t numThreads)    noexcept;
        static void destruct    ()                          noexcept;
        static void add_job     (Job* job)                  noexcept;
        static Job* get_job     ()                          noexcept;
        static void wait_for    (Job* job)                  noexcept;

        static std::span<JobSystemThread> get_threads() noexcept;

    private:
        static JobSystem* instance;

        std::span<JobSystemThread> threads;
        StaticThreadSafeQueue<Job*, EngineConfig::MAX_JOBS> jobs;

        friend JobSystemThread;

        JobSystem(std::size_t numThreads) noexcept;
        ~JobSystem() noexcept;

        void instance_add_job   (Job* job)  noexcept;
        Job* instance_get_job   ()          noexcept;
        void instance_wait_for  (Job* job)  noexcept;
    };
}

#endif
