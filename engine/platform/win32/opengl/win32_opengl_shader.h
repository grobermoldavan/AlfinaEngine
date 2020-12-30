#ifndef AL_WIN32_OPENGL_SHADER_H
#define AL_WIN32_OPENGL_SHADER_H

#include "win32_opengl_backend.h"

#include "engine/rendering/shader.h"

namespace al::engine
{
    template<> [[nodiscard]] Shader* create_shader<RendererType::OPEN_GL>(const char* vertexShaderSrc, const char* fragmentShaderSrc) noexcept;
    template<> void destroy_shader<RendererType::OPEN_GL>(Shader* shader) noexcept;

    class Win32OpenglShader : public Shader
    {
    public:
        Win32OpenglShader(const char* vertexShaderSrc, const char* fragmentShaderSrc) noexcept;
        ~Win32OpenglShader() noexcept;

        virtual void bind           () const noexcept override { ::glUseProgram(rendererId); }
        virtual void unbind         () const noexcept override { ::glUseProgram(0); }

        virtual void set_int        (const char* name, const int value)                     const noexcept override;
        virtual void set_int2       (const char* name, const int32_2& value)                const noexcept override;
        virtual void set_int3       (const char* name, const int32_3& value)                const noexcept override;
        virtual void set_int4       (const char* name, const int32_4& value)                const noexcept override;
        virtual void set_int_array  (const char* name, const int* values, uint32_t count)   const noexcept override;

        virtual void set_float      (const char* name, const float value)                   const noexcept override;
        virtual void set_float2     (const char* name, const float2& value)                 const noexcept override;
        virtual void set_float3     (const char* name, const float3& value)                 const noexcept override;
        virtual void set_float4     (const char* name, const float4& value)                 const noexcept override;
        virtual void set_float_array(const char* name, const float* values, uint32_t count) const noexcept override;

        // @TODO : add mat3 and implment this method
        // virtual void set_mat3        (const char* name, const float3x3& value)            const noexcept override;
        virtual void set_mat4        (const char* name, const float4x4& value)              const noexcept override;
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
        const char* sources[ShaderType::__size];

        void compile() noexcept;
        void handle_shader_compile_error(GLuint shaderId) const noexcept;
        void handle_program_compile_error(GLuint programId, GLenum(&shaderIds)[ShaderType::__size], bool(&isShaderIdSpecified)[ShaderType::__size]) const noexcept;

        struct GetUniformLocationResult
        {
            bool isFound;
            GLint location;
        };
        GetUniformLocationResult get_uniform_location(const char* name) const noexcept;
    };
}

#endif
