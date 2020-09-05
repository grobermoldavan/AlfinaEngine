#ifndef __ALFINA_TRANSFORM_H__
#define __ALFINA_TRANSFORM_H__

#include "math_types.h"

namespace al::engine
{
	class Transform
	{
	public:
		Transform();
		Transform(const float3& position, const float3& euler, const float3& scale);

		float4x4	get_matrix() const;

		float3		get_position() const;
		float3		get_rotation() const;
		float3		get_scale	() const;

		float3		get_forward	() const;
		float3		get_right	() const;
		float3		get_up		() const;

		void		set_position(const float3& position);
		void		set_rotation(const float3& euler);
		void		set_scale	(const float3& scale);

		void		set_position_mat(const float4x4& position);
		void		set_rotation_mat(const float4x4& rotation);
		void		set_scale_mat	(const float4x4& scale);

	private:
		float4x4 translationMat;
		float4x4 rotationMat;
		float4x4 scaleMat;
	};
}

#if defined(AL_UNITY_BUILD)
#	include "transform.cpp"
#else

#endif

#endif
