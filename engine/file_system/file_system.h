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
#include "utilities/array_container.h"

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
        static void                         construct           ()                                          noexcept;
        static void                         destruct            ()                                          noexcept;
        static [[nodiscard]] FileHandle*    sync_load           (std::string_view file, FileLoadMode mode)  noexcept;
        static [[nodiscard]] FileHandle*    async_load          (std::string_view file, FileLoadMode mode)  noexcept;
        static void                         free_handle         (FileHandle* handle)                        noexcept;
        static void                         remove_finished_jobs()                                          noexcept;

    private:
        static FileSystem* instance;

        ArrayContainer<FileHandle, EngineConfig::MAX_FILE_HANDLES>          handles;
        std::mutex                                                          handlesListMutex;
        // @NOTE :  SuList does not copy objects from one place another, so it is
        //          more suitable for storing Jobs because Jobs can't be copied
        SuList<AsyncFileReadJob, EngineConfig::MAX_ASYNC_FILE_READ_JOBS>    jobs;
        std::mutex                                                          jobsListMutex;
        AllocatorBase*                                                      allocator;
        Job                                                                 cleanupJob;

        FileSystem();
        ~FileSystem();

        [[nodiscard]] FileHandle*   instance_sync_load              (std::string_view file, FileLoadMode mode)  noexcept;
        [[nodiscard]] FileHandle*   instance_async_load             (std::string_view file, FileLoadMode mode)  noexcept;
        void                        instance_free_handle            (FileHandle* handle)                        noexcept;
        void                        instance_remove_finished_jobs   ()                                          noexcept;
        FileHandle*                 get_file_handle                 ()                                          noexcept;
        AsyncFileReadJob*           get_file_load_job               ()                                          noexcept;
    };
}

#endif
