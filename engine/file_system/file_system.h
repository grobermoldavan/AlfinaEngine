#ifndef AL_FILE_SYSTEM_H
#define AL_FILE_SYSTEM_H

#include <string_view>

#include "file_load.h"
#include "engine/memory/allocator_base.h"
#include "engine/job_system/job_system.h"
#include "utilities/static_unordered_list.h"

namespace al::engine
{
    class FileSystem
    {
    public:
        FileSystem(AllocatorBase* allocator);
        ~FileSystem();

        [[nodiscard]] FileHandle* sync_load(std::string_view file, FileLoadMode mode) noexcept;
        [[nodiscard]] FileHandle* async_load(std::string_view file, FileLoadMode mode) noexcept;
        void free_handle(FileHandle* handle) noexcept;

    private:
        SuList<FileHandle, 1024> handles;
        AllocatorBase* allocator;
    };

    FileSystem::FileSystem(AllocatorBase* allocator)
        : handles{ }
        , allocator{ allocator }
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
        al_assert_main_thread();

        FileHandle* handle = handles.get();
        *handle = al::engine::sync_load(file, allocator, mode);
        return handle;
    }

    [[nodiscard]] FileHandle* FileSystem::async_load(std::string_view file, FileLoadMode mode) noexcept
    {
        al_assert_main_thread();

        // Not implemented
        // @TODO : implement this when MemoryManager will
        //         be able to allocate memory in multi-thread
        al_assert(false);

        return nullptr;
    }

    void FileSystem::free_handle(FileHandle* handle) noexcept
    {
        al_assert_main_thread();

        allocator->deallocate(handle->memory, handle->size);
        handles.remove(handle);
    }
}

#endif
