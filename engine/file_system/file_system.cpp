
#include "file_system.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    FileSystem* gFileSystem = nullptr;

    void construct(FileSystem* fileSystem)
    {
        fileSystem->allocator = &gMemoryManager->pool;
    }

    void destruct(FileSystem* fileSystem)
    {
        
    }

    [[nodiscard]] FileHandle* file_sync_load(FileSystem* fileSystem, const StaticString& file, FileLoadMode mode)
    {
        al_profile_function();
        al_log_message( EngineConfig::FILE_SYSTEM_LOG_CATEGORY,
                        "Requested sync load file at path %s with mode %s",
                        cstr(&file), LOAD_MODE_TO_STR[static_cast<int>(mode)]);
        FileHandle* handle = fileSystem->allocator->allocate_and_construct<FileHandle>();
        *handle = al::engine::sync_load(cstr(&file), fileSystem->allocator, mode);
        return handle;
    }

    [[nodiscard]] HandleJobPair file_async_load(FileSystem* fileSystem, const StaticString& file, FileLoadMode mode)
    {
        al_profile_function();
        al_log_message( EngineConfig::FILE_SYSTEM_LOG_CATEGORY,
                        "Requested async load file at path %s with mode %s",
                        cstr(&file), LOAD_MODE_TO_STR[static_cast<int>(mode)]);
        FileHandle* handle = fileSystem->allocator->allocate_and_construct<FileHandle>();
        handle->state = FileHandle::State::LOADING;
        AsyncFileReadUserData* userData = fileSystem->allocator->allocate_and_construct<AsyncFileReadUserData>();
        construct(&userData->file, &file);
        userData->mode = mode;
        userData->handle = handle;
        Job* job = get_job(gMainJobSystem);
        configure(job, [fileSystem](Job* job)
        {
            AsyncFileReadUserData* userData = reinterpret_cast<AsyncFileReadUserData*>(job->userData);
            al_log_message( EngineConfig::FILE_SYSTEM_LOG_CATEGORY,
                            "Processing async load of file at path %s with mode %s",
                            cstr(&userData->file), LOAD_MODE_TO_STR[static_cast<int>(userData->mode)]);
            *userData->handle = al::engine::sync_load(cstr(&userData->file), fileSystem->allocator, userData->mode);
        }, userData);
        start_job(gMainJobSystem, job);
        return { handle, job };
    }

    void file_free_handle(FileSystem* fileSystem, FileHandle* handle)
    {
        al_profile_function();
        // @TODO : implement freeing currently loading handle
        al_assert(handle->state != FileHandle::State::LOADING);
        fileSystem->allocator->deallocate(handle->memory, handle->size);
        fileSystem->allocator->deallocate_as<FileHandle>(handle);
    }
}
