
#include "memory_manager.h"
#include "engine/config/engine_config.h"
#include "memory_common.h"

#include "utilities/constexpr_functions.h"

namespace al::engine
{
    MemoryManager MemoryManager::instance{ };

    MemoryManager::MemoryManager() noexcept
        : memory{ nullptr }
    { }

    MemoryManager::~MemoryManager() noexcept
    { }

    void MemoryManager::construct() noexcept
    {
        if (instance.memory)
        {
            return;
        }
        instance.memory = static_cast<std::byte*>(std::malloc(EngineConfig::MEMORY_SIZE + EngineConfig::DEFAULT_MEMORY_ALIGNMENT));
        instance.stack.initialize(align_pointer(instance.memory), EngineConfig::MEMORY_SIZE);
        // @TODO : change this hardcoded pool initialization to
        //         dynamic, which counts each bucket memory size
        //         based on some user settings
        instance.pool.initialize({
            bucket_desc(64                          , megabytes<std::size_t>(128)),
            bucket_desc(128                         , megabytes<std::size_t>(128)),
            bucket_desc(256                         , megabytes<std::size_t>(128)),
            bucket_desc(kilobytes<std::size_t>(1)   , megabytes<std::size_t>(128)),
            bucket_desc(kilobytes<std::size_t>(4)   , megabytes<std::size_t>(256)),
        }, &instance.stack);
    }

    void MemoryManager::destruct() noexcept
    {
        if (!instance.memory)
        {
            return;
        }
        std::free(instance.memory);
        instance.~MemoryManager();
    }

    inline StackAllocator* MemoryManager::get_stack() noexcept
    {
        return &instance.stack;
    }

    inline PoolAllocator* MemoryManager::get_pool() noexcept
    {
        return &instance.pool;
    }
}
