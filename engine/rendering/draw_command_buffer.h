#ifndef AL_DRAW_COMMAND_BUFFER_H
#define AL_DRAW_COMMAND_BUFFER_H

#include <cstdint>

#include "engine/config/engine_config.h"

#include "command_buffer.h"

#include "utilities/math.h"

namespace al::engine
{
    using DrawCommandKey = uint32_t;

    struct DrawCommandData : public CommandDataBase<DrawCommandKey>
    {
        Transform trf;
        class VertexArray* va;
        class Shader* shader;
        class Texture2d* texture;
    };

    using DrawCommandBuffer = CommandBuffer<DrawCommandKey, DrawCommandData, EngineConfig::DRAW_COMMAND_BUFFER_SIZE>;
}

#endif
