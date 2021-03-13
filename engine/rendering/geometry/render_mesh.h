#ifndef AL_GPU_MESH_H
#define AL_GPU_MESH_H

#include <cstdint> // for int's

#include "engine/config/engine_config.h"
#include "engine/containers/containers.h"
#include "engine/file_system/file_system.h"
#include "engine/rendering/render_core.h"

#include "utilities/math.h"
#include "utilities/array_container.h"
#include "utilities/stack.h"

namespace al::engine
{
    struct MeshVertex
    {
        float3 position;
        float3 normal;
        float2 uv;
    };

    struct RenderSubmesh
    {
        RendererIndexBufferHandle   ibHandle;
        RendererVertexBufferHandle  vbHandle;
        RendererVertexArrayHandle   vaHandle;
    };

    struct RenderMesh
    {
        ArrayContainer<RenderSubmesh, EngineConfig::RENDER_MESH_MAX_SUBMESHES> submeshes;
    };

    struct CpuSubmesh
    {
        StaticString name;
        DynamicArray<MeshVertex> vertices;
        DynamicArray<uint32_t> indices;
    };

    struct CpuMesh
    {
        ArrayContainer<CpuSubmesh, EngineConfig::RENDER_MESH_MAX_SUBMESHES> submeshes;
    };

    void fill_indices(CpuSubmesh* submesh);
    CpuMesh load_cpu_mesh_obj(FileHandle* handle);
}

#endif
