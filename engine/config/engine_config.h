#ifndef AL_ENGINE_CONFIG_H
#define AL_ENGINE_CONFIG_H

// @NOTE :  This header contains config information for engine.
//          Values can be changed by user if needed.

#include <cstddef>  // for std::size_t
#include <chrono>   // for std::chrono

#include "engine/platform/platform_file_system_config.h"
#include "engine/rendering/render_core.h"

#include "utilities/constexpr_functions.h"

namespace al::engine
{
    struct EngineConfig
    {
        // Thread settings
        static constexpr std::size_t                MAX_SUPPORTED_THREADS{ 64 }; // Bigger numbers of threads must be handled differently

        // Memory Manager settings
        static constexpr const char*                MEMORY_MANAGER_LOG_CATEGORY{ "Memory Manager" };

        static constexpr std::size_t                MEMORY_SIZE{ gigabytes<std::size_t>(1) };
        static constexpr std::size_t                POOL_ALLOCATOR_MEMORY_PERCENTS{ 80 };
        static constexpr std::size_t                POOL_ALLOCATOR_MAX_BUCKETS{ 5 };
        static constexpr std::size_t                POOL_ALLOCATOR_MAX_PTR_SIZE_PAIRS{ 64 };
        static constexpr std::size_t                DEFAULT_MEMORY_ALIGNMENT{ 8 };  // Bytes. Must be power of two

        // File System settings
        static constexpr const char*                FILE_SYSTEM_LOG_CATEGORY{ "File System" };

        static constexpr std::size_t                MAX_FILE_HANDLES{ 4096 };
        static constexpr std::size_t                MAX_ASYNC_FILE_READ_JOBS{ 128 };

        // Job System settings
        static constexpr const char*                JOB_SYSTEM_LOG_CATEGORY{ "Job System" };

        static constexpr std::size_t                MAX_JOBS{ 1024 };
        static constexpr std::size_t                MAX_NEXT_JOBS{ 64 };
        static constexpr std::chrono::nanoseconds   JOB_THREAD_SLEEP_TIME{ 100000 }; // 0.1 of millisecond

        // Log System settings
        static constexpr const char*                LOG_SYSTEM_LOG_CATEGORY{ "Log System" };
        static constexpr const char*                PROFILE_OUTPUT_FILE{ "profile.json" };
        static constexpr std::size_t                LOG_SYSTEM_BUFFER_SIZE{ megabytes<std::size_t>(8) };

        // Render System settings
        static constexpr const char*                RENDERER_LOG_CATEGORY{ "Renderer" };

        static constexpr std::size_t                FRAMEBUFFER_MAX_ATTACHMENTS{ 8 };
        static constexpr std::size_t                DRAW_COMMAND_BUFFER_SIZE{ 1024 };
        static constexpr std::size_t                RENDER_COMMAND_STACK_SIZE{ 1024 };
        static constexpr std::size_t                MAX_BUFFER_LAYOUT_ELEMENTS{ 16 };
        static constexpr RendererType               DEFAULT_RENDERER_TYPE{ RendererType::OPEN_GL };
        static constexpr const char*                SHADER_MODEL_MATRIX_UNIFORM_NAME{ "al_modelMatrix" };
        static constexpr const char*                SHADER_VIEW_PROJECTION_MATRIX_UNIFORM_NAME{ "al_vpMatrix" };

        static constexpr const char*                DRAW_FRAMEBUFFER_TO_SCREEN_VERT_SHADER_PATH{ "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "draw_framebuffer_to_screen.vert" };
        static constexpr const char*                DRAW_FRAMEBUFFER_TO_SCREEN_FRAG_SHADER_PATH{ "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "draw_framebuffer_to_screen.frag" };

        static constexpr const char*                DEFFERED_GEOMETRY_PASS_VERT_SHADER_PATH{ "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "deffered_geometry_pass.vert" };
        static constexpr const char*                DEFFERED_GEOMETRY_PASS_FRAG_SHADER_PATH{ "assets" AL_PATH_SEPARATOR "shaders" AL_PATH_SEPARATOR "deffered_geometry_pass.frag" };
        static constexpr uint32_t                   DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_LOCATION{ 0 };
        static constexpr const char*                DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_NAME{ "texDiffuse" };

        static constexpr uint32_t                   SCREEN_PASS_SOURCE_BUFFER_DIFFUSE_TEXTURE_LOCATION{ 0 };
        static constexpr const char*                SCREEN_PASS_SOURCE_BUFFER_DIFFUSE_TEXTURE_NAME{ "texScreenDiffuse" };

        static constexpr const char*                OPENGL_RENDERER_LOG_CATEGORY{ "OpenGL" };

        static constexpr std::size_t                RENDERER_MAX_INDEX_BUFFERS{ 1024 };
        static constexpr std::size_t                RENDERER_MAX_VERTEX_BUFFERS{ 1024 };
        static constexpr std::size_t                RENDERER_MAX_VERTEX_ARRAYS{ 1024 };
        static constexpr std::size_t                RENDERER_MAX_SHADERS{ 1024 };
        static constexpr std::size_t                RENDERER_MAX_FRAMEBUFFERS{ 1024 };
        static constexpr std::size_t                RENDERER_MAX_TEXTURES_2D{ 1024 };

        // ECS settings
        static constexpr const char*                ECS_LOG_CATEGORY{ "ECS" };

        static constexpr std::size_t                NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK{ 64 };

        // Scene settings
        static constexpr const char*                SCENE_LOG_CATEGORY{ "Scene" };

        static constexpr std::size_t                MAX_NUMBER_OF_SCENE_NODES{ 1024 };
        static constexpr std::size_t                MAX_NUMBER_OF_NODE_CHILDS{ 8 };
        static constexpr std::size_t                MAX_SCENE_NODE_NAME_LENGTH{ 64 };
        static constexpr const char*                SCENE_ROOT_NODE_NAME{ "RootNode" };

        // Containers settings
        static constexpr std::size_t                STATIC_STRING_LENGTH{ 256 };

        // Resource manager settings
        static constexpr const char*                RESOURCE_MANAGER_LOG_CATEGORY{ "Resource Manager" };

        static constexpr std::size_t                RESOURCE_MAX_TEXTURES{ 1024 };
        static constexpr std::size_t                RESOURCE_MAX_MESHES{ 1024 };

        // Mesh settings
        static constexpr std::size_t                CPU_MESH_DEFAULT_DYNAMIC_ARRAYS_SIZE{ 2048 };
        static constexpr std::size_t                RENDER_MESH_MAX_SUBMESHES{ 64 };
    };
}

#endif
