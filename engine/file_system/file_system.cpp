
#include "file_system.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    FileSystem* FileSystem::instance{ nullptr };

    FileSystem::FileSystem() noexcept
        : allocator{ MemoryManager::get_pool() }
    { }

    FileSystem::~FileSystem() noexcept
    { }

    void FileSystem::construct_system() noexcept
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

    FileSystem* FileSystem::get() noexcept
    {
        return instance;
    }
    
    [[nodiscard]] FileHandle* FileSystem::sync_load(const StaticString& file, FileLoadMode mode) noexcept
    {
        al_profile_function();
        al_log_message( EngineConfig::FILE_SYSTEM_LOG_CATEGORY,
                        "Requested sync load file at path %s with mode %s",
                        cstr(&file), LOAD_MODE_TO_STR[static_cast<int>(mode)]);
        FileHandle* handle = allocator->allocate_and_construct<FileHandle>();
        *handle = al::engine::sync_load(cstr(&file), allocator, mode);
        return handle;
    }

    [[nodiscard]] HandleJobPair FileSystem::async_load(const StaticString& file, FileLoadMode mode) noexcept
    {
        al_profile_function();
        al_log_message( EngineConfig::FILE_SYSTEM_LOG_CATEGORY,
                        "Requested async load file at path %s with mode %s",
                        cstr(&file), LOAD_MODE_TO_STR[static_cast<int>(mode)]);
        FileHandle* handle = allocator->allocate_and_construct<FileHandle>();
        handle->state = FileHandle::State::LOADING;
        AsyncFileReadUserData* userData = allocator->allocate_and_construct<AsyncFileReadUserData>();
        construct(&userData->file, &file);
        userData->mode = mode;
        userData->handle = handle;
        Job* job = JobSystem::get_main_system()->get_job();
        job->configure([this](Job* job)
        {
            AsyncFileReadUserData* userData = reinterpret_cast<AsyncFileReadUserData*>(job->get_user_data());
            al_log_message( EngineConfig::FILE_SYSTEM_LOG_CATEGORY,
                            "Processing async load of file at path %s with mode %s",
                            cstr(&userData->file), LOAD_MODE_TO_STR[static_cast<int>(userData->mode)]);
            *userData->handle = al::engine::sync_load(cstr(&userData->file), allocator, userData->mode);
        }, userData);
        JobSystem::get_main_system()->start_job(job);
        return { handle, job };
    }

    void FileSystem::free_handle(FileHandle* handle) noexcept
    {
        al_profile_function();
        // @TODO : implement freeing currently loading handle
        al_assert(handle->state != FileHandle::State::LOADING);
        allocator->deallocate(handle->memory, handle->size);
        allocator->deallocate_as<FileHandle>(handle);
    }
}
