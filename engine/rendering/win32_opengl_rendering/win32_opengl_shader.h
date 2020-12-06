#ifndef AL_WIN32_OPENGL_SHADER_H
#define AL_WIN32_OPENGL_SHADER_H

#include "win32_opengl_backend.h"

#include "../shader.h"
#include "engine/debug/debug.h"
#include "engine/containers/dynamic_array.h"
#include "engine/memory/memory_manager.h"

namespace al::engine
{
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

        static GLenum SHADER_TYPE_TO_GL_ENUM[ShaderType::__size];

        uint32_t    rendererId;
        const char* sources[ShaderType::__size];

        void compile() noexcept;
        void handle_shader_compile_error(GLuint shaderId) const noexcept;
        void handle_program_compile_error(GLuint programId, GLenum(&shaderIds)[ShaderType::__size], bool(&isShaderIdSpecified)[ShaderType::__size]) const noexcept;
        GLint get_uniform_location(const char* name) const noexcept;
    };

    GLenum Win32OpenglShader::SHADER_TYPE_TO_GL_ENUM[] =
    {
        GL_VERTEX_SHADER,
        GL_FRAGMENT_SHADER
    };

    Win32OpenglShader::Win32OpenglShader(const char* vertexShaderSrc, const char* fragmentShaderSrc) noexcept
    {
        sources[ShaderType::VERTEX] = vertexShaderSrc;
        sources[ShaderType::FRAGMENT] = fragmentShaderSrc;
        compile();
    }

    Win32OpenglShader::~Win32OpenglShader() noexcept
    {
        ::glDeleteProgram(rendererId);
    }

    void Win32OpenglShader::compile() noexcept
    {
        rendererId = ::glCreateProgram();
        al_assert(rendererId != 0); // Unable to create OpenGL program via glCreateProgram

        bool isShaderIdSpecified[ShaderType::__size];
        GLenum shaderIds[ShaderType::__size];

        for (size_t it = 0; it < ShaderType::__size; it++)
        {
            if (!sources[it])
            {
                isShaderIdSpecified[it] = false;
                continue;
            }
            
            ShaderType type = static_cast<ShaderType>(it);
            GLuint shaderId = ::glCreateShader(SHADER_TYPE_TO_GL_ENUM[type]);

            const GLchar* source = sources[it];
            ::glShaderSource(shaderId, 1, &source, 0);

            ::glCompileShader(shaderId);

            GLint isCompiled = 0;
            ::glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isCompiled);
            if (isCompiled != GL_TRUE) handle_shader_compile_error(shaderId);

            ::glAttachShader(rendererId, shaderId);
            isShaderIdSpecified[it] = true;
            shaderIds[it] = shaderId;
        }
        
        ::glLinkProgram(rendererId);

        GLint isLinked = 0;
        ::glGetProgramiv(rendererId, GL_LINK_STATUS, &isLinked);
        if (isLinked != GL_TRUE) handle_program_compile_error(rendererId, shaderIds, isShaderIdSpecified);

        for (size_t it = 0; it < ShaderType::__size; ++it)
        {
            if (!isShaderIdSpecified[it]) continue;
            ::glDetachShader(rendererId, shaderIds[it]);
            ::glDeleteShader(shaderIds[it]);
        }
    }

    void Win32OpenglShader::handle_shader_compile_error(GLuint shaderId) const noexcept
    {
        GLint infoLength = 0;
        ::glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLength);

        DynamicArray<GLchar> infoLog(infoLength);
        ::glGetShaderInfoLog(shaderId, infoLength, &infoLength, &infoLog[0]);
        ::glDeleteShader(shaderId);
        al_log_error("Shader1", "%s", &infoLog[0]);
    }

    void Win32OpenglShader::handle_program_compile_error(GLuint programId, GLenum(&shaderIds)[ShaderType::__size], bool(&isShaderIdSpecified)[ShaderType::__size]) const noexcept
    {
        GLint infoLength = 0;
        ::glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLength);

        DynamicArray<GLchar> infoLog(infoLength);
        ::glGetProgramInfoLog(programId, infoLength, &infoLength, &infoLog[0]);
        ::glDeleteProgram(programId);
        for (size_t it = 0; it < ShaderType::__size; ++it)
        {
            if(isShaderIdSpecified[it]) { ::glDeleteShader(shaderIds[it]); }
        }
        al_log_error("Shader2", "%s", &infoLog[0]);
    }

    GLint Win32OpenglShader::get_uniform_location(const char* name) const noexcept
    {
        GLint location = ::glGetUniformLocation(rendererId, name);
        if (location == -1)
        {
            al_log_error("Shader3", "Unable to find uniform location. Uniform name is %s", name);
            al_assert(location != -1);
        }
        return location;
    }

    void Win32OpenglShader::set_int(const char* name, const int value) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniform1i(location, value);
    }

    void Win32OpenglShader::set_int2(const char* name, const al::int32_2& value) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniform2i(location, value.x, value.y);
    }

    void Win32OpenglShader::set_int3(const char* name, const al::int32_3& value) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniform3i(location, value.x, value.y, value.z);
    }

    void Win32OpenglShader::set_int4(const char* name, const al::int32_4& value) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniform4i(location, value.x, value.y, value.z, value.w);
    }

    void Win32OpenglShader::set_int_array(const char* name, const int* values, uint32_t count) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniform1iv(location, count, values);
    }

    void Win32OpenglShader::set_float(const char* name, const float value) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniform1f(location, value);
    }

    void Win32OpenglShader::set_float2(const char* name, const al::float2& value) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniform2f(location, value.x, value.y);
    }

    void Win32OpenglShader::set_float3(const char* name, const al::float3& value) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniform3f(location, value.x, value.y, value.z);
    }

    void Win32OpenglShader::set_float4(const char* name, const al::float4& value) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniform4f(location, value.x, value.y, value.z, value.w);
    }

    void Win32OpenglShader::set_float_array(const char* name, const float* values, uint32_t count) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniform1fv(location, count, values);
    }

    void Win32OpenglShader::set_mat4(const char* name, const al::float4x4& value) const noexcept
    {
        GLint location = get_uniform_location(name);
        ::glUniformMatrix4fv(location, 1, GL_FALSE, value.elements);
    }

    template<> [[nodiscard]] Shader* create_shader<RendererType::OPEN_GL>(const char* vertexShaderSrc, const char* fragmentShaderSrc) noexcept
    {
        Shader* shader = MemoryManager::get()->get_pool()->allocate_as<Win32OpenglShader>();
        ::new(shader) Win32OpenglShader{ vertexShaderSrc, fragmentShaderSrc };
        return shader;
    }

    template<> void destroy_shader<RendererType::OPEN_GL>(Shader* shader) noexcept
    {
        shader->~Shader();
        MemoryManager::get()->get_pool()->deallocate(reinterpret_cast<std::byte*>(shader), sizeof(Win32OpenglShader));
    }
}

#endif
