
#include "mesh.h"

#include "engine/debug/debug.h"

#include "utilities/string_processing.h"
#define AL_SAFE_CAST_ASSERT(cond) al_assert(cond)
#include "utilities/safe_cast.h"

#define fill_vertex_data(v, word, array)                                              \
    for (uint32_t it = 0; it < (sizeof(decltype(v)::elements) / sizeof(float)); it++) \
    {                                                                                 \
        word = get_next_word({ word.data() + word.length() });                        \
        al_assert(word.length() > 0);                                                 \
        v.elements[it] = std::strtof(word.data(), nullptr);                           \
    }                                                                                 \
    array.push_back(v)

#define process_vertex(n, array)                \
    std::string_view word{ line.data(), 2 };    \
    float##n vert;                              \
    fill_vertex_data(vert, word, array)

namespace al::engine
{
    Mesh load_mesh_from_obj(FileHandle* handle) noexcept
    {
        al_profile_function();
        Mesh result{ };
        SubMesh* activeSubMesh = nullptr;
        // @TODO : reserve space in arrays
        DynamicArray<float3> vertices;
        DynamicArray<float3> normals;
        DynamicArray<float2> uvs;
        const char* fileText = reinterpret_cast<const char*>(handle->memory);
        for_each_line(fileText, [&](std::string_view line)
        {
            if (is_starts_with(line.data(), "v "))
            {
                al_assert(activeSubMesh);
                process_vertex(3, vertices);
            }
            else if (is_starts_with(line.data(), "vn "))
            {
                al_assert(activeSubMesh);
                process_vertex(3, normals);
            }
            else if (is_starts_with(line.data(), "vt "))
            {
                al_assert(activeSubMesh);
                process_vertex(2, uvs);
            }
            else if (is_starts_with(line.data(), "f "))
            {
                auto cast_from_obj = [](uint64_t arraySize, int64_t value, uint32_t* target)
                {
                    // OBJ format can't have zeros in face descriptions
                    al_assert(value != 0);
                    if (value > 0)
                    {
                        const bool castResult = safe_cast_int64_to_uint32(value - 1, target);
                        al_assert(castResult);
                    }
                    else
                    {
                        const bool castResult = safe_cast_uint64_to_uint32(arraySize + static_cast<uint64_t>(value), target);
                        al_assert(castResult);
                    }
                };
                al_assert(activeSubMesh);
                std::string_view word{ line.data(), 2 };
                // @NOTE :  Only triangulated meshes are supported
                for (uint32_t it = 0; it < 3; it++)
                {
                    uint32_t v = 0;
                    {
                        word = get_next_word(word.data() + word.length(), "/");
                        cast_from_obj(vertices.size(), std::strtoll(word.data(), nullptr, 10), &v);
                    }
                    uint32_t vt = 0;
                    if (uvs.size() > 0)
                    {
                        word = get_next_word(word.data() + word.length(), "/");
                        cast_from_obj(uvs.size(), std::strtoll(word.data(), nullptr, 10), &vt);
                    }
                    uint32_t vn = 0;
                    if (normals.size() > 0)
                    {
                        word = get_next_word(word.data() + word.length(), "/");
                        cast_from_obj(normals.size(), std::strtoll(word.data(), nullptr, 10), &vn);
                    }
                    activeSubMesh->vertices.push_back(
                    {
                        .position = vertices[v],
                        .normal   = normals[vn],
                        .uv       = uvs[vt]
                    });
                }
            }
            else if (is_starts_with(line.data(), "mtllib "))
            {

            }
            else if (is_starts_with(line.data(), "usemtl "))
            {

            }
            else if (is_starts_with(line.data(), "o "))
            {
                if (activeSubMesh)
                {
                    // @NOTE :  Triangle is needed to invert indices of triangles
                    //          because opengl expects them the other way around.
                    //          I'm not shure if it is common for all OBJ files, but
                    //          it is the thing with files exported from blender
                    uint32_t triangleIt = 0;
                    for (uint32_t it = 0; it < activeSubMesh->vertices.size(); it++)
                    {
                        // 0 : 2, 1 : 0, 2 : -2
                        int32_t additional = (triangleIt == 0) ? 2 : (triangleIt == 1) ? 0 : -2;
                        activeSubMesh->indices.push_back(activeSubMesh->indices.size() + additional);
                        triangleIt = (triangleIt + 1) % 3;
                    }
                }
                activeSubMesh = result.subMeshes.get();
                al_assert(activeSubMesh);
                std::string_view subMeshName = get_next_word(line.data() + 2);
                activeSubMesh->name = subMeshName.data();
                vertices.clear();
                normals.clear();
                uvs.clear();
            }
            else if (is_starts_with(line.data(), "g "))
            {
                // Ignored ?
            }
        });
        return result;
    }
}
