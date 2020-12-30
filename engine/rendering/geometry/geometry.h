#ifndef AL_GEOMETRY_H
#define AL_GEOMETRY_H

#include <cstdint>
#include <cstring>

#include "engine/rendering/texture_2d.h"
#include "engine/containers/containers.h"
#include "engine/file_system/file_system.h"
#include "engine/debug/debug.h"

#include "utilities/math.h"

namespace al::engine
{
    struct GeometryVertex
    {
        float3 position;
        float3 normal;
        float2 uv;
    };

    struct Geometry
    {
        DynamicArray<GeometryVertex> vertices;
        DynamicArray<uint32_t> ids;
    };

    Geometry load_geometry_from_obj(FileHandle* handle);
}

#endif
