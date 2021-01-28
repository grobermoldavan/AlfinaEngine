#ifndef AL_GEOMETRY_COMMAND_BUFFER_H
#define AL_GEOMETRY_COMMAND_BUFFER_H

#include <cstdint>

#include "engine/config/engine_config.h"

#include "command_buffer.h"

#include "utilities/math.h"

namespace al::engine
{
    using GeometryCommandKey = uint32_t;

    struct GeometryCommandData : public CommandDataBase<GeometryCommandKey>
    {
        deprecated_Transform trf;
        class VertexArray* va;
        class Texture2d* diffuseTexture;
        class Texture2d* specularTexture;
    };

    using GeometryCommandBuffer = CommandBuffer<GeometryCommandKey, GeometryCommandData, EngineConfig::DRAW_COMMAND_BUFFER_SIZE>;
}

#endif
