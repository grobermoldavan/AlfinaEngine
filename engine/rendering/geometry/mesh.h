#ifndef AL_MESH_H
#define AL_MESH_H

#include "engine/containers/containers.h"
#include "engine/file_system/file_system.h"
#include "engine/rendering/index_buffer.h"
#include "engine/rendering/vertex_buffer.h"
#include "engine/rendering/vertex_array.h"

#include "utilities/math.h"
#include "utilities/fixed_size_string.h"
#include "utilities/array_container.h"

namespace al::engine
{
    struct MeshVertex
    {
        float3 position;
        float3 normal;
        float2 uv;
    };

    struct SubMesh
    {
        DynamicArray<MeshVertex> vertices;
        DynamicArray<uint32_t> indices;
        FixedSizeString<128> name;
        IndexBuffer* ib{ nullptr };
        VertexBuffer* vb{ nullptr };
        VertexArray* va{ nullptr };
    };
    
    struct Mesh
    {
        ArrayContainer<SubMesh, 16> subMeshes;
    };

    Mesh load_mesh_from_obj(FileHandle* handle) noexcept;
}

#endif
