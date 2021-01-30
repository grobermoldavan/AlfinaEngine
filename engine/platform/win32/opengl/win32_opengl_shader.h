#ifndef AL_WIN32_OPENGL_SHADER_H
#define AL_WIN32_OPENGL_SHADER_H

#include <string_view>

#include "win32_opengl_backend.h"

#include "engine/rendering/shader.h"

namespace al::engine
{
    template<> [[nodiscard]] Shader* create_shader<RendererType::OPEN_GL>(std::string_view vertexShaderSrc, std::string_view fragmentShaderSrc) noexcept;
    template<> void destroy_shader<RendererType::OPEN_GL>(Shader* shader) noexcept;

    class Win32OpenglShader : public Shader
    {
    public:
        Win32OpenglShader(std::string_view vertexShaderSrc, std::string_view fragmentShaderSrc) noexcept;
        ~Win32OpenglShader() noexcept;

        virtual void bind           () const noexcept override { ::glUseProgram(rendererId); }
        virtual void unbind         () const noexcept override { ::glUseProgram(0); }

        virtual void set_int        (std::string_view name, const int value)                     const noexcept override;
        virtual void set_int2       (std::string_view name, const int32_2& value)                const noexcept override;
        virtual void set_int3       (std::string_view name, const int32_3& value)                const noexcept override;
        virtual void set_int4       (std::string_view name, const int32_4& value)                const noexcept override;
        virtual void set_int_array  (std::string_view name, const int* values, uint32_t count)   const noexcept override;

        virtual void set_float      (std::string_view name, const float value)                   const noexcept override;
        virtual void set_float2     (std::string_view name, const float2& value)                 const noexcept override;
        virtual void set_float3     (std::string_view name, const float3& value)                 const noexcept override;
        virtual void set_float4     (std::string_view name, const float4& value)                 const noexcept override;
        virtual void set_float_array(std::string_view name, const float* values, uint32_t count) const noexcept override;

        // @TODO : add mat3 and implment this method
        // virtual void set_mat3        (std::string_view name, const float3x3& value)            const noexcept override;
        virtual void set_mat4        (std::string_view name, const float4x4& value)              const noexcept override;
    private:
        enum ShaderType
        {
            VERTEX,
            FRAGMENT,
            __size
        };

        constexpr static GLenum SHADER_TYPE_TO_GL_ENUM[ShaderType::__size] =
        {
            GL_VERTEX_SHADER,
            GL_FRAGMENT_SHADER
        };

        RendererId  rendererId;
        std::string_view sources[ShaderType::__size];

        void compile() noexcept;
        void handle_shader_compile_error(GLuint shaderId) const noexcept;
        void handle_program_compile_error(GLuint programId, GLenum(&shaderIds)[ShaderType::__size], bool(&isShaderIdSpecified)[ShaderType::__size]) const noexcept;

        struct GetUniformLocationResult
        {
            bool isFound;
            GLint location;
        };
        GetUniformLocationResult get_uniform_location(std::string_view name) const noexcept;
    };
}

#endif
