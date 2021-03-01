#ifndef AL_RESOURCE_MANAGER_H
#define AL_RESOURCE_MANAGER_H

#include <cstdint> // for ints

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

    struct RenderMeshComponent
    {
        MeshResourceHandle resourceHandle;
    };

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

        MeshResourceHandle add_mesh_resource(const StaticString& path);
        MeshResourceHandle get_mesh_resource(const StaticString& path);
        RenderMesh* get_render_mesh(MeshResourceHandle handle);

    private:
        static ResourceManager* instance;

        struct al_align TextureResource
        {
            StaticString path;
            RendererTexture2dHandle rendererHandle;
        };

        struct al_align MeshResource
        {
            StaticString path;
            CpuMesh cpuMesh;
            RenderMesh renderMesh;
        };

        SuList<TextureResource, EngineConfig::RESOURCE_MAX_TEXTURES> textureResources;
        SuList<MeshResource, EngineConfig::RESOURCE_MAX_MESHES> meshResources;
    };
}

#endif
