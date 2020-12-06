#ifndef AL_FILE_SYSTEM_H
#define AL_FILE_SYSTEM_H

#include <string_view>
#include <mutex>

#include "engine/config/engine_config.h"

#include "file_load.h"
#include "engine/memory/memory_manager.h"
#include "engine/job_system/job_system.h"
#include "engine/debug/debug.h"

#include "utilities/static_unordered_list.h"

/*
    @TODO : Main goal - add ability to async load files in file system.
            In order to achieve this we need a way to allocate memory
            in a thread-safe (ideally in a lock-free) manner.
            1)  First solution is to have thread-local pool allocators.
                This is not the best way because we would need to somehow
                balance allocations between all threads, to prevent one
                thread from running out of memory.
            2)  Second solution is to use regular mutex for allocation and
                deallocations. But this is slow.
            3)  Third solution is to implement lock-free allocator :
                https://www.cs.tufts.edu/~nr/cs257/archive/maged-michael/pldi-2004.pdf
            
            Also we need a thread-safe (ideally lock-free) data structure
            that gives us ability to add elements, remove given elements and
            iterate through elements. Probably thread-safe version of SuList
            will be good. This is needed to store FileHandles as well as
            async load jobs in file system.
            1)  First solution is to use mutexes. Slow.
            2)  Other solution is to implement lock-free lists
                https://habr.com/ru/post/317882/
                https://en.wikipedia.org/wiki/Non-blocking_linked_list
*/

namespace al::engine
{
    class AsyncFileReadJob : public Job
    {
    public:
        using Job::Job;

        char fileName[EngineConfig::ASYNC_FILE_READ_JOB_FILE_NAME_SIZE];
        FileLoadMode mode;
        FileHandle* handle;
    };

    class FileSystem
    {
    public:
        FileSystem(JobSystem* jobSystem);
        ~FileSystem();

        [[nodiscard]] FileHandle* sync_load(std::string_view file, FileLoadMode mode) noexcept;
        [[nodiscard]] FileHandle* async_load(std::string_view file, FileLoadMode mode) noexcept;
        void free_handle(FileHandle* handle) noexcept;

        void remove_finished_jobs() noexcept;

    private:
        SuList<FileHandle, EngineConfig::MAX_FILE_HANDLES> handles;
        std::mutex handlesListMutex;

        SuList<AsyncFileReadJob, EngineConfig::MAX_ASYNC_FILE_READ_JOBS> jobs;
        std::mutex jobsListMutex;

        AllocatorBase* allocator;
        JobSystem* jobSystem;
        Job cleanupJob;

        FileHandle* get_file_handle() noexcept;
        AsyncFileReadJob* get_file_load_job() noexcept;
    };

    FileSystem::FileSystem(JobSystem* jobSystem)
        : handles{ }
        , allocator{ MemoryManager::get()->get_pool() }
        , jobSystem{ jobSystem }
    { }

    FileSystem::~FileSystem()
    {
        handles.for_each([this](FileHandle* handle)
        {
            allocator->deallocate(handle->memory, handle->size);
        });
    }

    [[nodiscard]] FileHandle* FileSystem::sync_load(std::string_view file, FileLoadMode mode) noexcept
    {
        al_profile_function();

        FileHandle* handle = get_file_handle();
        *handle = al::engine::sync_load(file, allocator, mode);
        return handle;
    }

    [[nodiscard]] FileHandle* FileSystem::async_load(std::string_view file, FileLoadMode mode) noexcept
    {
        al_profile_function();
        al_assert(EngineConfig::ASYNC_FILE_READ_JOB_FILE_NAME_SIZE > file.size());

        FileHandle* handle = get_file_handle();
        handle->state = FileHandle::State::LOADING;

        AsyncFileReadJob* job = get_file_load_job();
        ::new(job) AsyncFileReadJob
        {
            [this](Job* baseJob)
            {
                AsyncFileReadJob* job = static_cast<AsyncFileReadJob*>(baseJob);
                *job->handle = al::engine::sync_load(job->fileName, allocator, job->mode);
            }
        };

        job->handle = handle;
        job->mode = mode;
        std::memcpy(job->fileName, file.data(), file.size());
        job->fileName[file.size()] = 0;

        jobSystem->add_job(job);

        return handle;
    }

    void FileSystem::free_handle(FileHandle* handle) noexcept
    {
        al_profile_function();

        // @TODO : implement freeing currently loading handle
        al_assert(handle->state != FileHandle::State::LOADING);

        allocator->deallocate(handle->memory, handle->size);
        {
            std::lock_guard<std::mutex> lock{ handlesListMutex };
            handles.remove(handle);
        }
    }

    FileHandle* FileSystem::get_file_handle() noexcept
    {
        al_profile_function();

        FileHandle* handle = nullptr;
        {
            std::lock_guard<std::mutex> lock{ handlesListMutex };
            handle = handles.get();
        }
        // Out of handles
        al_assert(handle);
        return handle;
    }

    AsyncFileReadJob* FileSystem::get_file_load_job() noexcept
    {
        al_profile_function();

        AsyncFileReadJob* job = nullptr;
        {
            std::lock_guard<std::mutex> lock{ jobsListMutex };
            job = jobs.get();
        }
        // Out of jobs
        al_assert(job);
        return job;
    }

    // @NOTE :  after async load job has been finished it will not be removed
    //          until this method is called. So, this method must be called if
    //          there is no more jobs left in sulist, or regularly (each frame
    //          every other frame, for example)
    void FileSystem::remove_finished_jobs() noexcept
    {
        al_profile_function();

        // Wait for cleanup if job is still running
        jobSystem->wait_for(&cleanupJob);

        // Clean up job simply removes all finished jobs from the list
        ::new(&cleanupJob) Job
        {
            [this](Job*)
            {
                std::lock_guard<std::mutex> lock{ jobsListMutex };
                jobs.remove_by_condition([](AsyncFileReadJob* job) -> bool
                {
                    return job->is_finished();
                });
            }
        };

        jobSystem->add_job(&cleanupJob);
    }
}

#endif
