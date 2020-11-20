#ifndef AL_FILE_SYSTEM_H
#define AL_FILE_SYSTEM_H

#include <string_view>

#include "file_load.h"
#include "engine/memory/allocator_base.h"
#include "engine/job_system/job_system.h"
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
    class FileSystem
    {
    public:
        FileSystem(AllocatorBase* allocator);
        ~FileSystem();

        [[nodiscard]] FileHandle* sync_load(std::string_view file, FileLoadMode mode) noexcept;
        [[nodiscard]] FileHandle* async_load(std::string_view file, FileLoadMode mode) noexcept;
        void free_handle(FileHandle* handle) noexcept;

    private:
        SuList<FileHandle, 4096> handles;
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
