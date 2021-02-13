
#include "resource_manager.h"

#include "engine/memory/memory_manager.h"

namespace al::engine
{
    ResourceManager* ResourceManager::instance{ nullptr };

    void ResourceManager::construct() noexcept
    {
        if (instance)
        {
            return;
        }
        instance = MemoryManager::get_stack()->allocate_and_construct<ResourceManager>();
    }

    void ResourceManager::destruct() noexcept
    {
        if (!instance)
        {
            return;
        }
        instance->~ResourceManager();
    }

    ResourceManager* ResourceManager::get() noexcept
    {
        return instance;
    }
}
