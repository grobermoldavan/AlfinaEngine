#ifndef __ALFINA_WINDOWS_OPENGL_SHADER_H__
#define __ALFINA_WINDOWS_OPENGL_SHADER_H__

#include "../base_shader.h"

#include "engine/math/math.h"
#include "utilities/dispensable.h"

namespace al::engine
{
	class Win32glShader : public Shader
	{
	public:
		Win32glShader(const char* vertexShaderSrc, const char* fragmentShaderSrc);
		~Win32glShader();

		virtual void bind	() const override { ::glUseProgram(rendererId); }
		virtual void unbind	() const override { ::glUseProgram(0); }

		virtual void set_int		(const char* name, const int value)						const override;
		virtual void set_int2		(const char* name, const int32_2& value)				const override;
		virtual void set_int3		(const char* name, const int32_3& value)				const override;
		virtual void set_int4		(const char* name, const int32_4& value)				const override;
		virtual void set_int_array	(const char* name, const int* values, uint32_t count)	const override;

		virtual void set_float		(const char* name, const float value)					const override;
		virtual void set_float2		(const char* name, const float2& value)					const override;
		virtual void set_float3		(const char* name, const float3& value)					const override;
		virtual void set_float4		(const char* name, const float4& value)					const override;
		virtual void set_float_array(const char* name, const float* values, uint32_t count)	const override;

		virtual void set_mat3		(const char* name, const float3x3& value)				const override;
		virtual void set_mat4		(const char* name, const float4x4& value)				const override;

	private:
		enum ShaderType
		{
			VERTEX,
			FRAGMENT,
			__size
		};

		static GLenum SHADER_TYPE_TO_GL_ENUM[ShaderType::__size];

		uint32_t					rendererId;
		Dispensable<const char*>	sources[ShaderType::__size];

		void compile();
		void handle_shader_compile_error(GLuint shaderId);
		void handle_program_compile_error(GLuint programId, Dispensable<GLenum>(&shaderIds)[ShaderType::__size]);
	};
}

#include "windows_opengl_shader.cpp"

#endif
