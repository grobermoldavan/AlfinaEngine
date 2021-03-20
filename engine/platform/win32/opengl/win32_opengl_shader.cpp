
#include "win32_opengl_shader.h"

#include "engine/debug/debug.h"
#include "engine/memory/memory_manager.h"
#include "engine/containers/containers.h"

namespace al::engine
{
    namespace internal
    {
        template<> [[nodiscard]] Shader* create_shader<RendererType::OPEN_GL>(const ShaderInitData& initData) noexcept
        {
            Win32OpenglShader* shader = static_cast<Win32OpenglShader*>(allocate(&gMemoryManager->pool, sizeof(Win32OpenglShader)));
            wrap_construct(shader, initData);
            return shader;
        }

        template<> void destroy_shader<RendererType::OPEN_GL>(Shader* shader) noexcept
        {
            wrap_destruct(shader);
            deallocate(&gMemoryManager->pool, shader, sizeof(Win32OpenglShader));
        }
    }

    Win32OpenglShader::Win32OpenglShader(const ShaderInitData& initData) noexcept
    {
        sources[ShaderType::VERTEX] = initData.vertexShaderSrc;
        sources[ShaderType::FRAGMENT] = initData.fragmentShaderSrc;
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
            if (sources[it].empty())
            {
                isShaderIdSpecified[it] = false;
                continue;
            }

            ShaderType type = static_cast<ShaderType>(it);
            GLuint shaderId = ::glCreateShader(SHADER_TYPE_TO_GL_ENUM[type]);

            const GLchar* source = sources[it].data();
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
        DynamicArray<GLchar> infoLog;
        construct(&infoLog);
        expand(&infoLog, infoLength);
        ::glGetShaderInfoLog(shaderId, infoLength, &infoLength, infoLog.memory);
        ::glDeleteShader(shaderId);
        al_log_error("Shader", "%s", infoLog.memory);
        destruct(&infoLog);
    }

    void Win32OpenglShader::handle_program_compile_error(GLuint programId, GLenum(&shaderIds)[ShaderType::__size], bool(&isShaderIdSpecified)[ShaderType::__size]) const noexcept
    {
        GLint infoLength = 0;
        ::glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLength);

        DynamicArray<GLchar> infoLog;
        construct(&infoLog);
        expand(&infoLog, infoLength);
        ::glGetProgramInfoLog(programId, infoLength, &infoLength, infoLog.memory);
        ::glDeleteProgram(programId);
        for (size_t it = 0; it < ShaderType::__size; ++it)
        {
            if(isShaderIdSpecified[it]) { ::glDeleteShader(shaderIds[it]); }
        }
        al_log_error("Shader", "%s", infoLog.memory);
        destruct(&infoLog);
    }

    Win32OpenglShader::GetUniformLocationResult Win32OpenglShader::get_uniform_location(std::string_view name) const noexcept
    {
        GLint location = ::glGetUniformLocation(rendererId, name.data());
        if (location == -1)
        {
            al_log_error("Shader", "Unable to find uniform location. Uniform name is %s", name);
            return { false, 0 };
        }
        return { true, location };
    }

    void Win32OpenglShader::set_int(std::string_view name, const int value) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniform1i(location, value); }
    }

    void Win32OpenglShader::set_int2(std::string_view name, const al::int32_2& value) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniform2i(location, value.x, value.y); }
    }

    void Win32OpenglShader::set_int3(std::string_view name, const al::int32_3& value) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniform3i(location, value.x, value.y, value.z); }
    }

    void Win32OpenglShader::set_int4(std::string_view name, const al::int32_4& value) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniform4i(location, value.x, value.y, value.z, value.w); }
    }

    void Win32OpenglShader::set_int_array(std::string_view name, const int* values, uint32_t count) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniform1iv(location, count, values); }
    }

    void Win32OpenglShader::set_float(std::string_view name, const float value) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniform1f(location, value); }
    }

    void Win32OpenglShader::set_float2(std::string_view name, const al::float2& value) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniform2f(location, value.x, value.y); }
    }

    void Win32OpenglShader::set_float3(std::string_view name, const al::float3& value) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniform3f(location, value.x, value.y, value.z); }
    }

    void Win32OpenglShader::set_float4(std::string_view name, const al::float4& value) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniform4f(location, value.x, value.y, value.z, value.w); }
    }

    void Win32OpenglShader::set_float_array(std::string_view name, const float* values, uint32_t count) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniform1fv(location, count, values); }
    }

    void Win32OpenglShader::set_mat4(std::string_view name, const al::float4x4& value) const noexcept
    {
        auto [isFound, location] = get_uniform_location(name);
        if (isFound) { ::glUniformMatrix4fv(location, 1, GL_FALSE, value.elements); }
    }
}
