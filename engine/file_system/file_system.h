#ifndef AL_FILE_SYSTEM_H
#define AL_FILE_SYSTEM_H

#include "engine/config/engine_config.h"

#include "file_load.h"
#include "engine/platform/platform_file_system_utilities.h"
#include "engine/memory/memory_manager.h"
#include "engine/job_system/job_system.h"
#include "engine/containers/containers.h"

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
    struct AsyncFileReadUserData
    {
        StaticString file;
        FileLoadMode mode;
        FileHandle* handle;
    };

    struct HandleJobPair
    {
        FileHandle* handle;
        Job* job;
    };

    class FileSystem
    {
    public:
        static void        construct() noexcept;
        static void        destruct() noexcept;
        static FileSystem* get() noexcept;

        [[nodiscard]] FileHandle*   sync_load   (const StaticString& file, FileLoadMode mode)   noexcept;
        [[nodiscard]] HandleJobPair async_load  (const StaticString& file, FileLoadMode mode)   noexcept;
        void                        free_handle (FileHandle* handle)                            noexcept;

    private:
        static FileSystem* instance;

        AllocatorBase* allocator;

        FileSystem() noexcept;
        ~FileSystem() noexcept;
    };
}

#endif
