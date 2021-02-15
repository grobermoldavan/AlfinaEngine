
#include "render_mesh.h"

#include "engine/debug/debug.h"

#include "utilities/string_processing.h"
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
    CpuMesh load_cpu_mesh_obj(FileHandle* handle) noexcept
    {
        al_profile_function();
        CpuMesh result{ };
        CpuSubmesh* activeSubmesh = nullptr;
        const char* fileText = reinterpret_cast<const char*>(handle->memory);
        DynamicArray<float3> positions; positions.reserve(EngineConfig::CPU_MESH_DEFAULT_DYNAMIC_ARRAYS_SIZE);
        DynamicArray<float3> normals;   normals  .reserve(EngineConfig::CPU_MESH_DEFAULT_DYNAMIC_ARRAYS_SIZE);
        DynamicArray<float2> uvs;       uvs      .reserve(EngineConfig::CPU_MESH_DEFAULT_DYNAMIC_ARRAYS_SIZE);
        for_each_line(fileText, [&](std::string_view line)
        {
            if (is_starts_with(line.data(), "v "))
            {
                al_assert(activeSubmesh);
                process_vertex(3, positions);
            }
            else if (is_starts_with(line.data(), "vn "))
            {
                al_assert(activeSubmesh);
                process_vertex(3, normals);
            }
            else if (is_starts_with(line.data(), "vt "))
            {
                al_assert(activeSubmesh);
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
                al_assert(activeSubmesh);
                std::string_view word{ line.data(), 2 };
                // @NOTE :  Only triangulated meshes are supported
                for (uint32_t it = 0; it < 3; it++)
                {
                    uint32_t v = 0;
                    {
                        word = get_next_word(word.data() + word.length(), "/");
                        cast_from_obj(positions.size(), std::strtoll(word.data(), nullptr, 10), &v);
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
                    activeSubmesh->vertices.push_back
                    ({
                        .position = positions[v],
                        .normal   = normals[vn],
                        .uv       = uvs[vt]
                    });
                }
            }
            else if (is_starts_with(line.data(), "mtllib "))
            {
                // Ignored ?
            }
            else if (is_starts_with(line.data(), "usemtl "))
            {
                // Ignored ?
            }
            else if (is_starts_with(line.data(), "o "))
            {
                activeSubmesh = result.submeshes.get();
                al_assert(activeSubmesh);
                std::string_view submeshName = get_next_word(line.data() + 2);
                activeSubmesh->name.set_with_length(submeshName.data(), submeshName.length());
                positions.clear();
                normals.clear();
                uvs.clear();
            }
            else if (is_starts_with(line.data(), "g "))
            {
                // Ignored ?
            }
        });
        return std::move(result);
    }
}
