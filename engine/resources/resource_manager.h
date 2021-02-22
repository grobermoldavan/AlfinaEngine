#ifndef AL_RESOURCE_MANAGER_H
#define AL_RESOURCE_MANAGER_H

#include <cstdint> // for uints

#include "engine/config/engine_config.h"
#include "engine/containers/containers.h"
#include "engine/rendering/renderer.h"
#include "engine/memory/memory_common.h"

#include "utilities/flags.h"
#include "utilities/static_unordered_list.h"

namespace al::engine
{
    struct ResourceManagerHandleT
    {
        union
        {
            struct
            {
                uint64_t isValid : 1;
                uint64_t index : 63;
            };
            uint64_t value;
        };
    };
    using TextureResourceHandle = ResourceManagerHandleT;
    using MeshResourceHandle    = ResourceManagerHandleT;

    class ResourceManager
    {
    public:
        static void construct() noexcept;
        static void destruct() noexcept;
        static ResourceManager* get() noexcept;

        ResourceManager() noexcept;
        ~ResourceManager() noexcept;

        TextureResourceHandle add_texture_resource(const StaticString& path);
        TextureResourceHandle get_texture_resource(const StaticString& path);
        RendererTexture2dHandle get_renderer_texture_handle(TextureResourceHandle handle);

    private:
        static ResourceManager* instance;

        static constexpr const char* LOG_CATEGORY_RESOURCE_MANAGER = "Resource Manager";

        struct al_align TextureResource
        {
            StaticString path;
            RendererTexture2dHandle rendererHandle;
        };

        SuList<TextureResource, EngineConfig::RESOURCE_MAX_TEXTURES> textureResources;

    };
}

#endif
