
#include "resource_manager.h"

#include "engine/memory/memory_manager.h"
#include "engine/debug/debug.h"

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

    ResourceManager::ResourceManager() noexcept
        : textureResources{ }
    { }

    ResourceManager::~ResourceManager() noexcept
    { }

    TextureResourceHandle ResourceManager::add_texture_resource(const StaticString& path)
    {
        al_profile_function();
        al_log_message(LOG_CATEGORY_RESOURCE_MANAGER, "Adding texture resource at path : \"%s\"", path);
        TextureResourceHandle handle{ 0 };
        // @NOTE :  Check if this texture is already registered. It true, simply
        //          return handle to already registered resource
        handle = get_texture_resource(path);
        if (handle.value)
        {
            al_log_message(LOG_CATEGORY_RESOURCE_MANAGER, "Resource is already added to the resource manager. Returning handle");
        }
        else
        {
            al_profile_scope("Create resource");
            TextureResource* resource = textureResources.get();
            handle.isValid = 1;
            handle.index = textureResources.get_direct_index(resource);
            resource->path = path;
            resource->rendererHandle = Renderer::get()->reserve_texture_2d();
            Renderer::get()->create_texture_2d(resource->rendererHandle, static_cast<const char*>(resource->path));
        }
        return handle;
    }

    TextureResourceHandle ResourceManager::get_texture_resource(const StaticString& path)
    {
        al_profile_function();
        TextureResourceHandle handle{ 0 };
        textureResources.for_each_interruptible([&](TextureResource* resource)
        {
            if (resource->path == path)
            {
                handle.isValid = 1;
                handle.index = textureResources.get_direct_index(resource);
                return false;
            }
            return true;
        });
        return handle;
    }

    RendererTexture2dHandle ResourceManager::get_renderer_texture_handle(TextureResourceHandle handle)
    {
        if (!handle.value)
        {
            return { 0 };
        }
        return textureResources.direct_accsess(handle.index)->rendererHandle;
    }
}
