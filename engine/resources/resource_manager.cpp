
#include <cstddef> // for std::size_t

#include "resource_manager.h"

#include "engine/memory/memory_manager.h"
#include "engine/debug/debug.h"
#include "engine/file_system/file_system.h"
#include "engine/job_system/job_system.h"
#include "utilities/procedural_wrap.h"

namespace al::engine
{
    ResourceManager* ResourceManager::instance{ nullptr };

    void ResourceManager::construct_manager() noexcept
    {
        if (instance)
        {
            return;
        }
        instance = static_cast<ResourceManager*>(allocate(&gMemoryManager->stack, sizeof(ResourceManager)));
        wrap_construct(instance);
    }

    void ResourceManager::destruct() noexcept
    {
        if (!instance)
        {
            return;
        }
        wrap_destruct(instance);
    }

    // Renamed it for now. Anyway this will be removed later
    ResourceManager* ResourceManager::get_instance() noexcept
    {
        return instance;
    }

    ResourceManager::ResourceManager() noexcept
        : textureResources{ }
        , meshResources{ }
    { }

    ResourceManager::~ResourceManager() noexcept
    { }

    TextureResourceHandle ResourceManager::add_texture_resource(const StaticString& path)
    {
        al_profile_function();
        al_log_message(EngineConfig::RESOURCE_MANAGER_LOG_CATEGORY, "Adding texture resource at path : \"%s\"", cstr(&path));
        TextureResourceHandle handle{ 0 };
        // @NOTE :  Check if this texture is already registered. It true, simply
        //          return handle to already registered resource
        handle = get_texture_resource(path);
        if (handle.isValid)
        {
            al_log_message(EngineConfig::RESOURCE_MANAGER_LOG_CATEGORY, "Resource is already added to the resource manager. Returning handle");
        }
        else
        {
            al_profile_scope("Create texture resource");
            TextureResource* resource = textureResources.get();
            handle.isValid = 1;
            handle.index = textureResources.get_direct_index(resource);
            construct(&resource->path, &path);
            resource->rendererHandle = Renderer::get()->reserve_texture_2d();
            Renderer::get()->create_texture_2d(resource->rendererHandle, { cstr(&resource->path) });
        }
        return handle;
    }

    TextureResourceHandle ResourceManager::get_texture_resource(const StaticString& path)
    {
        al_profile_function();
        TextureResourceHandle handle{ 0 };
        textureResources.for_each_interruptible([&](TextureResource* resource)
        {
            if (is_equal(&resource->path, &path))
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
        if (!handle.isValid)
        {
            return { 0 };
        }
        return textureResources.direct_accsess(handle.index)->rendererHandle;
    }

    MeshResourceHandle ResourceManager::add_mesh_resource(const StaticString& path)
    {
        al_profile_function();
        al_log_message(EngineConfig::RESOURCE_MANAGER_LOG_CATEGORY, "Adding mesh resource at path : \"%s\"", cstr(&path));
        MeshResourceHandle handle{ 0 };
        // @NOTE :  Check if this mesh is already registered. It true, simply
        //          return handle to already registered resource
        handle = get_mesh_resource(path);
        if (handle.isValid)
        {
            al_log_message(EngineConfig::RESOURCE_MANAGER_LOG_CATEGORY, "Resource is already added to the resource manager. Returning handle");
        }
        else
        {
            al_profile_scope("Create mesh resource");
            // @NOTE :  Step 1. Get new mesh resource and create a handle for it
            MeshResource* resource = meshResources.get();
            handle.isValid = 1;
            handle.index = meshResources.get_direct_index(resource);
            construct(&resource->path, &path);
            al_memzero(&resource->renderMesh.submeshes);
            // @NOTE :  Step 2. Start loading that resource
            auto [fileHandle, loadJob] = file_async_load(gFileSystem, path, FileLoadMode::READ);
            // @NOTE :  Step 3. Start post load job
            Job* postLoadJob = get_job(gMainJobSystem);
            configure(postLoadJob, [fileHandle, resource](Job* job)
            {
                // @NOTE :  This job should not be executed on render thread.
                al_assert(!Renderer::get()->is_render_thread());
                al_log_message(EngineConfig::RESOURCE_MANAGER_LOG_CATEGORY, "Loading mesh");
                // @NOTE :  Step 4. Load mesh data from file
                resource->cpuMesh = load_cpu_mesh_obj(fileHandle);
                file_free_handle(gFileSystem, fileHandle);
                // @NOTE :  Step 5. Process loaded submeshes (aka generate render mesh)
                for_each_array_container(resource->cpuMesh.submeshes, it)
                {
                    al_profile_scope("Proces cpu submesh");
                    CpuSubmesh* submesh = get(&resource->cpuMesh.submeshes, it);
                    al_log_message(EngineConfig::RESOURCE_MANAGER_LOG_CATEGORY, "Processing submesh with name %s", submesh->name);
                    // @NOTE :  Step 6. Reserve index buffer, vertex buffer and vertex array handles
                    RenderSubmesh* renderSubmesh = push(&resource->renderMesh.submeshes);
                    renderSubmesh->ibHandle = Renderer::get()->reserve_index_buffer();
                    renderSubmesh->vbHandle = Renderer::get()->reserve_vertex_buffer();
                    renderSubmesh->vaHandle = Renderer::get()->reserve_vertex_array();
                    // @NOTE :  Step 7. Start create render resource job
                    Job* createRenderResourcesJob = get_job(gRenderJobSystem);
                    configure(createRenderResourcesJob, [renderSubmesh, submesh](Job*)
                    {
                        al_profile_scope("Create submesh render resources");
                        al_log_message(EngineConfig::RESOURCE_MANAGER_LOG_CATEGORY, "Creating render resources for submesh with name %s", submesh->name);
                        al_log_message(EngineConfig::RESOURCE_MANAGER_LOG_CATEGORY, "Submesh %s : number of vertices : %d", submesh->name, submesh->vertices.size);
                        // @NOTE :  Step 8. Actually create buffers and vertex array based on loaded mesh data
                        Renderer::get()->create_index_buffer(renderSubmesh->ibHandle, { submesh->indices.memory, submesh->indices.size });
                        Renderer::get()->create_vertex_buffer(renderSubmesh->vbHandle, { submesh->vertices.memory, submesh->vertices.size * sizeof(MeshVertex) });
                        VertexBuffer* vb = Renderer::get()->vertex_buffer(renderSubmesh->vbHandle);
                        vb->set_layout(BufferLayout::ElementContainer
                        {
                            BufferElement{ Float3, false }, // position
                            BufferElement{ Float3, false }, // normal
                            BufferElement{ Float2, false }  // uv
                        });
                        Renderer::get()->create_vertex_array(renderSubmesh->vaHandle, { });
                        VertexArray* va = Renderer::get()->vertex_array(renderSubmesh->vaHandle);
                        va->set_vertex_buffer(vb);
                        va->set_index_buffer(Renderer::get()->index_buffer(renderSubmesh->ibHandle));
                        // @NOTE :  When all createRenderResourcesJob instances (for each submesh) will be dispatched,
                        //          then mesh will be ready for a render

                        // @NOTE :  Maybe we need to destruct cpu submesh, thus erasing all mesh information from it ?
                        // submesh->~CpuSubmesh();
                    });
                    start_job(gRenderJobSystem, createRenderResourcesJob);
                }
            });
            set_after(postLoadJob, loadJob);
            start_job(gMainJobSystem, postLoadJob);
        }
        return handle;
    }

    MeshResourceHandle ResourceManager::get_mesh_resource(const StaticString& path)
    {
        al_profile_function();
        MeshResourceHandle handle{ 0 };
        meshResources.for_each_interruptible([&](MeshResource* resource)
        {
            if (is_equal(&resource->path, &path))
            {
                handle.isValid = 1;
                handle.index = meshResources.get_direct_index(resource);
                return false;
            }
            return true;
        });
        return handle;
    }

    RenderMesh* ResourceManager::get_render_mesh(MeshResourceHandle handle)
    {
        if (!handle.isValid)
        {
            return nullptr;
        }
        return &meshResources.direct_accsess(handle.index)->renderMesh;
    }
}
