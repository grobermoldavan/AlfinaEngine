#ifndef AL_RESOURCE_MANAGER_H
#define AL_RESOURCE_MANAGER_H

#include <cstdint>

#include "engine/config/engine_config.h"
#include "engine/containers/containers.h"
#include "engine/rendering/renderer.h"

namespace al::engine
{
    using ResourceManagerIndex = uint64_t;

    class ResourceManager
    {
    public:
        static void construct() noexcept;
        static void destruct() noexcept;
        static ResourceManager* get() noexcept;

        // void add_texture_resource(ResourceManagerIndex textureIndex);

    private:
        static ResourceManager* instance;

        // struct TextureResource
        // {
        //     StaticString path;
        //     RendererTexture2dHandle rendererHandle;
        // };

        // TextureResource textures[EngineConfig::MAX_TEXTURES];

    };
}

#endif
