#ifndef AL_SHADER_H
#define AL_SHADER_H

#include <cstdint>

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

        virtual void set_int        (const char* name, const int value)                     const noexcept = 0;
        virtual void set_int2       (const char* name, const int32_2& value)                const noexcept = 0;
        virtual void set_int3       (const char* name, const int32_3& value)                const noexcept = 0;
        virtual void set_int4       (const char* name, const int32_4& value)                const noexcept = 0;
        virtual void set_int_array  (const char* name, const int* values, uint32_t count)   const noexcept = 0;

        virtual void set_float      (const char* name, const float value)                   const noexcept = 0;
        virtual void set_float2     (const char* name, const float2& value)                 const noexcept = 0;
        virtual void set_float3     (const char* name, const float3& value)                 const noexcept = 0;
        virtual void set_float4     (const char* name, const float4& value)                 const noexcept = 0;
        virtual void set_float_array(const char* name, const float* values, uint32_t count) const noexcept = 0;

        // @TODO : add mat3 and implment this method
        // virtual void set_mat3        (const char* name, const float3x3& value)            const noexcept = 0;
        virtual void set_mat4        (const char* name, const float4x4& value)              const noexcept = 0;
    };

    template<RendererType type> [[nodiscard]] Shader* create_shader(const char* vertexShaderSrc, const char* fragmentShaderSrc) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
        return nullptr;
    }

    template<RendererType type> void destroy_shader(Shader* shader) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
    }
}

#endif
