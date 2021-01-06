#ifndef AL_ENGINE_CONFIG_H
#define AL_ENGINE_CONFIG_H

// @NOTE :  This header contains config information for engine.
//          Values can be changed by user if needed.

#include <cstddef>
#include <chrono>

#include "engine/rendering/render_core.h"

#include "utilities/constexpr_functions.h"

namespace al::engine
{
    struct EngineConfig
    {
        // Memory Manager settings
        static constexpr std::size_t                MEMORY_SIZE{ gigabytes<std::size_t>(1) };
        static constexpr std::size_t                POOL_ALLOCATOR_MAX_BUCKETS{ 5 };
        static constexpr std::size_t                POOL_ALLOCATOR_MAX_PTR_SIZE_PAIRS{ 64 };

        // File System settings
        static constexpr std::size_t                MAX_FILE_HANDLES{ 4096 };
        static constexpr std::size_t                MAX_ASYNC_FILE_READ_JOBS{ 128 };
        static constexpr std::size_t                ASYNC_FILE_READ_JOB_FILE_NAME_SIZE{ 128 };

        // Job System settings
        static constexpr std::size_t                MAX_JOBS{ 1024 };
        static constexpr std::chrono::nanoseconds   JOB_THREAD_SLEEP_TIME{ 100000 }; // 0.1 of millisecond

        // Log System settings
        static constexpr std::size_t                LOG_BUFFER_SIZE{ megabytes<std::size_t>(1) };
        static constexpr bool                       LOG_USE_DEFAULT_OUTPUT{ true };
        static constexpr const char*                LOG_OUTPUT_FILE{ "log.txt" };
        static constexpr std::size_t                PROFILE_BUFFER_SIZE{ megabytes<std::size_t>(2) };
        static constexpr bool                       PROFILE_USE_DEFAULT_OUTPUT{ false };
        static constexpr const char*                PROFILE_OUTPUT_FILE{ "profile.json" };

        // Render System settings
        static constexpr std::size_t                FRAMEBUFFER_MAX_ATTACHMENTS{ 8 };
        static constexpr std::size_t                DRAW_COMMAND_BUFFER_SIZE{ 1024 };
        static constexpr std::size_t                RENDER_COMMAND_STACK_SIZE{ 1024 };
        static constexpr std::size_t                MAX_BUFFER_LAYOUT_ELEMENTS{ 16 };
        static constexpr RendererType               DEFAULT_RENDERER_TYPE{ RendererType::OPEN_GL };
        static constexpr const char*                SHADER_MODEL_MATRIX_UNIFORM_NAME{ "al_modelMatrix" };
        static constexpr const char*                SHADER_VIEW_PROJECTION_MATRIX_UNIFORM_NAME{ "al_vpMatrix" };

        static constexpr const char*                DRAW_FRAMEBUFFER_TO_SCREEN_VERT_SHADER_PATH{ "assets\\shaders\\draw_framebuffer_to_screen.vert" };
        static constexpr const char*                DRAW_FRAMEBUFFER_TO_SCREEN_FRAG_SHADER_PATH{ "assets\\shaders\\draw_framebuffer_to_screen.frag" };

        static constexpr const char*                DEFFERED_GEOMETRY_PASS_VERT_SHADER_PATH{ "assets\\shaders\\deffered_geometry_pass.vert" };
        static constexpr const char*                DEFFERED_GEOMETRY_PASS_FRAG_SHADER_PATH{ "assets\\shaders\\deffered_geometry_pass.frag" };
        static constexpr uint32_t                   DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_LOCATION{ 0 };
        static constexpr const char*                DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_NAME{ "texDiffuse" };

        static constexpr uint32_t                   SCREEN_PASS_SOURCE_BUFFER_TEXTURE_LOCATION{ 0 };
        static constexpr const char*                SCREEN_PASS_SOURCE_BUFFER_TEXTURE_NAME{ "texScreen" };

        static constexpr const char*                OPENGL_RENDERER_LOG_CATEGORY = "OpenGL";
    };
}

#endif
