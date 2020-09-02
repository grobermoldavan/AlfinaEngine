#if defined(AL_UNITY_BUILD)

#else
#	include "windows_opengl_shader.h"
#endif

namespace al::engine
{
	ErrorInfo create_shader(Shader** shader, const char* vertexShaderSrc, const char* fragmentShaderSrc)
	{
		*shader = static_cast<Shader*>(new Win32glShader(vertexShaderSrc, fragmentShaderSrc));
		return{ ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo destroy_shader(const Shader* shader)
	{
		if (shader) delete shader;
		return{ ErrorInfo::Code::ALL_FINE };
	}

	GLenum Win32glShader::SHADER_TYPE_TO_GL_ENUM[] =
	{
		GL_VERTEX_SHADER,
		GL_FRAGMENT_SHADER
	};

	Win32glShader::Win32glShader(const char* vertexShaderSrc, const char* fragmentShaderSrc)
	{
		sources[ShaderType::VERTEX] = vertexShaderSrc;
		sources[ShaderType::FRAGMENT] = fragmentShaderSrc;
		compile();
	}

	Win32glShader::~Win32glShader()
	{
		::glDeleteProgram(rendererId);
	}

	void Win32glShader::compile()
	{
		rendererId = ::glCreateProgram();
		AL_ASSERT_MSG_NO_DISCARD(rendererId != 0, "Unable to create OpenGL program via glCreateProgram")

		al::Dispensable<GLenum> shaderIds[ShaderType::__size];

		for (size_t it = 0; it < ShaderType::__size; ++it)
		{
			if (!sources[it].is_specified()) continue;

			ShaderType type = static_cast<ShaderType>(it);
			GLuint shaderId = ::glCreateShader(SHADER_TYPE_TO_GL_ENUM[type]);

			const GLchar* source = sources[it];
			::glShaderSource(shaderId, 1, &source, 0);

			::glCompileShader(shaderId);

			GLint isCompiled = 0;
			::glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled != GL_TRUE) handle_shader_compile_error(shaderId);

			::glAttachShader(rendererId, shaderId);
			shaderIds[it] = shaderId;
		}
		
		::glLinkProgram(rendererId);

		GLint isLinked = 0;
		::glGetProgramiv(rendererId, GL_LINK_STATUS, &isLinked);
		if (isLinked != GL_TRUE) handle_program_compile_error(rendererId, shaderIds);

		for (auto shaderId : shaderIds)
		{
			if (!shaderId.is_specified()) continue;

			::glDetachShader(rendererId, shaderId);
			::glDeleteShader(shaderId);
		}
	}

	void Win32glShader::handle_shader_compile_error(GLuint shaderId)
	{
		GLint infoLength = 0;
		::glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLength);

		std::vector<GLchar> infoLog(infoLength);
		::glGetShaderInfoLog(shaderId, infoLength, &infoLength, &infoLog[0]);

		::glDeleteShader(shaderId);

		AL_LOG_NO_DISCARD(Logger::Type::ERROR_MSG, &infoLog[0])
	}

	void Win32glShader::handle_program_compile_error(GLuint programId, al::Dispensable<GLenum>(&shaderIds)[ShaderType::__size])
	{
		GLint infoLength = 0;
		::glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLength);

		std::vector<GLchar> infoLog(infoLength);
		::glGetProgramInfoLog(programId, infoLength, &infoLength, &infoLog[0]);

		::glDeleteProgram(programId);

		for (auto shaderId : shaderIds)
		{
			if (shaderId.is_specified())
				::glDeleteShader(shaderId);
		}

		AL_LOG_NO_DISCARD(Logger::Type::ERROR_MSG, &infoLog[0])
	}

	void Win32glShader::set_int(const char* name, const int value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform1i(location, value);
	}

	void Win32glShader::set_int2(const char* name, const al::int32_2& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform2i(location, value[0], value[1]);
	}

	void Win32glShader::set_int3(const char* name, const al::int32_3& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform3i(location, value[0], value[1], value[2]);
	}

	void Win32glShader::set_int4(const char* name, const al::int32_4& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform4i(location, value[0], value[1], value[2], value[3]);
	}

	void Win32glShader::set_int_array(const char* name, const int* values, uint32_t count) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform1iv(location, count, values);
	}

	void Win32glShader::set_float(const char* name, const float value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform1f(location, value);
	}

	void Win32glShader::set_float2(const char* name, const al::float2& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform2f(location, value[0], value[1]);
	}

	void Win32glShader::set_float3(const char* name, const al::float3& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform3f(location, value[0], value[1], value[2]);
	}

	void Win32glShader::set_float4(const char* name, const al::float4& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform4f(location, value[0], value[1], value[2], value[3]);
	}

	void Win32glShader::set_float_array(const char* name, const float* values, uint32_t count) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform1fv(location, count, values);
	}

	void Win32glShader::set_mat3(const char* name, const al::float3x3& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniformMatrix3fv(location, 1, GL_FALSE, value.data_ptr());
	}

	void Win32glShader::set_mat4(const char* name, const al::float4x4& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniformMatrix4fv(location, 1, GL_FALSE, value.data_ptr());
	}
}
