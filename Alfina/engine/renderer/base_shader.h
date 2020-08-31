#ifndef __ALFINA_BASE_SHADER_H__
#define __ALFINA_BASE_SHADER_H__

#include "engine/math/math.h"
#include "engine/engine_utilities/error_info.h"

namespace al::engine
{
	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void bind			() const = 0;
		virtual void unbind			() const = 0;

		virtual void set_int		(const char* name, const int value)						const = 0;
		virtual void set_int2		(const char* name, const al::int32_2& value)			const = 0;
		virtual void set_int3		(const char* name, const al::int32_3& value)			const = 0;
		virtual void set_int4		(const char* name, const al::int32_4& value)			const = 0;
		virtual void set_int_array	(const char* name, const int* values, uint32_t count)	const = 0;

		virtual void set_float		(const char* name, const float value)					const = 0;
		virtual void set_float2		(const char* name, const al::float2& value)				const = 0;
		virtual void set_float3		(const char* name, const al::float3& value)				const = 0;
		virtual void set_float4		(const char* name, const al::float4& value)				const = 0;
		virtual void set_float_array(const char* name, const float* values, uint32_t count)	const = 0;

		virtual void set_mat3		(const char* name, const al::float3x3& value)			const = 0;
		virtual void set_mat4		(const char* name, const al::float4x4& value)			const = 0;
	};

	extern ErrorInfo create_shader	(Shader**, const char* vertexShaderSrc, const char* fragmentShaderSrc);
	extern ErrorInfo destroy_shader	(const Shader*);
}

#endif