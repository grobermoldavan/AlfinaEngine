#ifndef AL_SHADER_H
#define AL_SHADER_H

#include <cstdint>
#include <string_view>

#include "render_core.h"
#include "engine/debug/debug.h"

#include "utilities/math.h"

namespace al::engine
{
    class Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void bind           () const noexcept = 0;
        virtual void unbind         () const noexcept = 0;

        virtual void set_int        (std::string_view name, const int value)                     const noexcept = 0;
        virtual void set_int2       (std::string_view name, const int32_2& value)                const noexcept = 0;
        virtual void set_int3       (std::string_view name, const int32_3& value)                const noexcept = 0;
        virtual void set_int4       (std::string_view name, const int32_4& value)                const noexcept = 0;
        virtual void set_int_array  (std::string_view name, const int* values, uint32_t count)   const noexcept = 0;

        virtual void set_float      (std::string_view name, const float value)                   const noexcept = 0;
        virtual void set_float2     (std::string_view name, const float2& value)                 const noexcept = 0;
        virtual void set_float3     (std::string_view name, const float3& value)                 const noexcept = 0;
        virtual void set_float4     (std::string_view name, const float4& value)                 const noexcept = 0;
        virtual void set_float_array(std::string_view name, const float* values, uint32_t count) const noexcept = 0;

        // @TODO : add mat3 and implment this method
        // virtual void set_mat3        (std::string_view name, const float3x3& value)            const noexcept = 0;
        virtual void set_mat4        (std::string_view name, const float4x4& value)              const noexcept = 0;
    };

    namespace internal
    {
        template<RendererType type> [[nodiscard]] Shader* create_shader(std::string_view vertexShaderSrc, std::string_view fragmentShaderSrc) noexcept
        {
            al_log_error(EngineConfig::RENDERER_LOG_CATEGORY, "Unsupported rendering API");
            al_assert(false);
            return nullptr;
        }

        template<RendererType type> void destroy_shader(Shader* shader) noexcept
        {
            al_log_error(EngineConfig::RENDERER_LOG_CATEGORY, "Unsupported rendering API");
            al_assert(false);
        }

        [[nodiscard]] Shader* create_shader(RendererType type, std::string_view vertexShaderSrc, std::string_view fragmentShaderSrc) noexcept;
        void destroy_shader(RendererType type, Shader* shader) noexcept;
    }
}

#endif
