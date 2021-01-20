
#include "file_system.h"

namespace al::engine
{
    FileSystem* FileSystem::instance{ nullptr };

    FileSystem::FileSystem()
        : handles{ }
        , allocator{ MemoryManager::get_pool() }
    { }

    FileSystem::~FileSystem()
    {
        handles.for_each([this](FileHandle* handle)
        {
            allocator->deallocate(handle->memory, handle->size);
        });
    }

    void FileSystem::construct() noexcept
    {
        if (instance)
        {
            return;
        }
        instance = MemoryManager::get_stack()->allocate_as<FileSystem>();
        ::new(instance) FileSystem{ };
    }

    void FileSystem::destruct() noexcept
    {
        if (!instance)
        {
            return;
        }
        instance->~FileSystem();
    }

    inline [[nodiscard]] FileHandle* FileSystem::sync_load(std::string_view file, FileLoadMode mode) noexcept
    {
        return instance->instance_sync_load(file, mode);
    }

    inline [[nodiscard]] FileHandle* FileSystem::async_load(std::string_view file, FileLoadMode mode) noexcept
    {
        return instance->instance_async_load(file, mode);
    }

    inline void FileSystem::free_handle(FileHandle* handle) noexcept
    {
        instance->instance_free_handle(handle);
    }

    inline void FileSystem::remove_finished_jobs() noexcept
    {
        instance->instance_remove_finished_jobs();
    }

    [[nodiscard]] FileHandle* FileSystem::instance_sync_load(std::string_view file, FileLoadMode mode) noexcept
    {
        al_profile_function();
        FileHandle* handle = get_file_handle();
        *handle = al::engine::sync_load(file, allocator, mode);
        return handle;
    }

    [[nodiscard]] FileHandle* FileSystem::instance_async_load(std::string_view file, FileLoadMode mode) noexcept
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
        JobSystem::add_job(job);
        return handle;
    }

    void FileSystem::instance_free_handle(FileHandle* handle) noexcept
    {
        al_profile_function();
        // @TODO : implement freeing currently loading handle
        al_assert(handle->state != FileHandle::State::LOADING);
        allocator->deallocate(handle->memory, handle->size);
        {
            std::lock_guard<std::mutex> lock{ handlesListMutex };
            handles.remove_by_condition([&](FileHandle* containerHandle) -> bool
            {
                return containerHandle == handle;
            });
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
    void FileSystem::instance_remove_finished_jobs() noexcept
    {
        al_profile_function();
        // Wait for cleanup if job is still running
        JobSystem::wait_for(&cleanupJob);
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
        JobSystem::add_job(&cleanupJob);
    }
}
