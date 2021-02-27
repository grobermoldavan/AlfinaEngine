
#include "render_mesh.h"

#include "engine/debug/debug.h"
#include "engine/job_system/job_system.h"

#include "utilities/string_processing.h"
#include "utilities/safe_cast.h"
#include "utilities/stack.h"

namespace al::engine
{
    void fill_indices(CpuSubmesh* submesh) noexcept
    {
        // @NOTE :  Triangle is needed to invert indices of triangles
        //          because opengl expects them the other way around.
        //          I'm not shure if it is common for all OBJ files, but
        //          it is the thing with files exported from blender
        const std::size_t size = submesh->vertices.size();
        submesh->indices.reserve(size);
        uint32_t triangleIt = 0;
        for (uint32_t it = 0; it < submesh->vertices.size(); it++)
        {
            // 0 : 2, 1 : 0, 2 : -2
            int32_t additional = (triangleIt == 0) ? 2 : (triangleIt == 1) ? 0 : -2;
            submesh->indices.push_back(submesh->indices.size() + additional);
            triangleIt = (triangleIt + 1) % 3;
        }
    }

    CpuMesh load_cpu_mesh_obj(FileHandle* handle) noexcept
    {
        al_profile_function();
        CpuMesh result{ };
        const char* fileText = reinterpret_cast<const char*>(handle->memory);
        const char* fileTextPtr = fileText;
        CpuSubmesh* activeSubmesh = nullptr;
        DynamicArray<float3> positions; positions.reserve(EngineConfig::CPU_MESH_DEFAULT_DYNAMIC_ARRAYS_SIZE);
        DynamicArray<float3> normals;   normals  .reserve(EngineConfig::CPU_MESH_DEFAULT_DYNAMIC_ARRAYS_SIZE);
        DynamicArray<float2> uvs;       uvs      .reserve(EngineConfig::CPU_MESH_DEFAULT_DYNAMIC_ARRAYS_SIZE);
        while(true)
        {
            if (is_starts_with(fileTextPtr, "v "))
            {
                al_assert(activeSubmesh);
                // @NOTE :  works only with three-component vectors
                float3 position
                {
                    std::strtof((fileTextPtr = advance_to_next_word(fileTextPtr), fileTextPtr), nullptr),
                    std::strtof((fileTextPtr = advance_to_next_word(fileTextPtr), fileTextPtr), nullptr),
                    std::strtof((fileTextPtr = advance_to_next_word(fileTextPtr), fileTextPtr), nullptr)
                };
                positions.push_back(position);
            }
            else if (is_starts_with(fileTextPtr, "vn "))
            {
                al_assert(activeSubmesh);
                // @NOTE :  works only with three-component vectors
                float3 normal
                {
                    std::strtof((fileTextPtr = advance_to_next_word(fileTextPtr), fileTextPtr), nullptr),
                    std::strtof((fileTextPtr = advance_to_next_word(fileTextPtr), fileTextPtr), nullptr),
                    std::strtof((fileTextPtr = advance_to_next_word(fileTextPtr), fileTextPtr), nullptr)
                };
                normals.push_back(normal);
            }
            else if (is_starts_with(fileTextPtr, "vt "))
            {
                al_assert(activeSubmesh);
                // @NOTE :  works only with two-component vectors
                float2 uv
                {
                    std::strtof((fileTextPtr = advance_to_next_word(fileTextPtr), fileTextPtr), nullptr),
                    std::strtof((fileTextPtr = advance_to_next_word(fileTextPtr), fileTextPtr), nullptr)
                };
                uvs.push_back(uv);
            }
            else if (is_starts_with(fileTextPtr, "f "))
            {
                auto cast_from_obj = [](uint64_t arraySize, int64_t value, uint32_t* target)
                {
                    // @NOTE :  OBJ format can't have zeros in face descriptions
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
                for (uint32_t it = 0; it < 3; it++)
                {
                    uint32_t v = 0;
                    {
                        fileTextPtr = advance_to_next_word(fileTextPtr, '/');
                        cast_from_obj(positions.size(), std::strtoll(fileTextPtr, nullptr, 10), &v);
                    }
                    uint32_t vt = 0;
                    if (uvs.size() > 0)
                    {
                        fileTextPtr = advance_to_next_word(fileTextPtr, '/');
                        cast_from_obj(uvs.size(), std::strtoll(fileTextPtr, nullptr, 10), &vt);
                    }
                    uint32_t vn = 0;
                    if (normals.size() > 0)
                    {
                        fileTextPtr = advance_to_next_word(fileTextPtr, '/');
                        cast_from_obj(normals.size(), std::strtoll(fileTextPtr, nullptr, 10), &vn);
                    }
                    activeSubmesh->vertices.push_back
                    ({
                        .position = positions[v],
                        .normal   = normals[vn],
                        .uv       = uvs[vt]
                    });
                }
            }
            else if (is_starts_with(fileTextPtr, "mtllib "))
            {
                // Ignored ?
            }
            else if (is_starts_with(fileTextPtr, "usemtl "))
            {
                // Ignored ?
            }
            // @NOTE :  Currently we treat objects ("o " lines) and groups ("g " lines) the same
            else if (is_starts_with(fileTextPtr, "o ") /*|| is_starts_with(fileTextPtr, "g ")*/)
            {
                if (activeSubmesh)
                {
                    fill_indices(activeSubmesh);
                }
                activeSubmesh = result.submeshes.get();
                al_assert(activeSubmesh);
                fileTextPtr = advance_to_next_word(fileTextPtr);
                const char* submeshNameStart = fileTextPtr;
                fileTextPtr = advance_to_word_ending(fileTextPtr);
                const char* submeshNameEnd = fileTextPtr;
                activeSubmesh->name.set_with_length(submeshNameStart, submeshNameEnd - submeshNameStart);
                positions.clear();
                normals.clear();
                uvs.clear();
            }
            const char* prevFileTextPtr = fileTextPtr;
            fileTextPtr = advance_to_next_line(fileTextPtr);
            if (prevFileTextPtr == fileTextPtr)
            {
                break;
            }
        }
        if (activeSubmesh)
        {
            fill_indices(activeSubmesh);
        }
        return std::move(result);
    }
}
