#if defined(AL_UNITY_BUILD)

#else
#	include "render_camera.h"
#endif

#include <cmath>

#include "utilities/constexpr_functions.h"

namespace al::engine
{
	PerspectiveRenderCamera::PerspectiveRenderCamera()
		: transform		{ }
		, aspectRatio	{ 4.0f / 3.0f }
		, nearPlane		{ 0.1f }
		, farPlane		{ 100.0f }
		, fovDeg		{ 45.0f }
	{ }

	PerspectiveRenderCamera::PerspectiveRenderCamera
	(
		Transform	_transform,
		float2		_aspectRatio,
		float		_nearPlane,
		float		_farPlane,
		float		_fovDeg
	)
		: transform		{ _transform }
		, aspectRatio	{ _aspectRatio[0] / _aspectRatio[1] }
		, nearPlane		{ _nearPlane }
		, farPlane		{ _farPlane }
		, fovDeg		{ _fovDeg }
	{ }

	float4x4 PerspectiveRenderCamera::get_projection() const
	{
		float4x4 result;

#if 0
		float halfFov = to_radians(fovDeg) / 2.0f;

		float top		= nearPlane * std::tan(halfFov);
		float bottom	= -1.0f * top;
		float right		= top * aspectRatio;
		float left		= -1.0f * right;

		float sx = 2.0f * nearPlane / (right - left);
		float sy = 2.0f * nearPlane / (top - bottom);

		float c2 = /*-1.0f **/ (farPlane + nearPlane) / (farPlane - nearPlane);
		float c1 = 2.0f * nearPlane * farPlane / (nearPlane - farPlane);

		float tx = -1.0f * nearPlane * (left + right) / (right - left);
		float ty = -1.0f * nearPlane * (bottom + top) / (top - bottom);

		result[0][0] = sx;
		result[1][1] = sy;
		result[2][2] = c2;
		result[3][2] = /*-1.0f*/ 1.0f;
		result[0][3] = tx;
		result[1][3] = ty;
		result[2][3] = c1;
#else
		float tanHalfFov = std::tan(to_radians(fovDeg) / 2.0f);
		result[0][0] = 1.0f / (aspectRatio * tanHalfFov);
		result[1][1] = 1.0f / tanHalfFov;
		result[2][2] = /*-1.0f*/ 1.0f * (farPlane + nearPlane) / (farPlane - nearPlane);
		result[3][2] = /*-1.0f*/ 1.0f;
		result[2][3] = -1.0f * (2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
#endif

		return result;
	}

	float4x4 PerspectiveRenderCamera::get_view() const
	{
		return transform.get_matrix().invert();
	}

	void PerspectiveRenderCamera::set_position(const float3& position)
	{
		transform.set_position(position);
	}

	void PerspectiveRenderCamera::look_at(const float3& target, const float3& up)
	{
		float3 forward		= (target - transform.get_position()).normalized();
		float3 right		= forward.cross(up).normalized();
		float3 correctedUp	= right.cross(forward).normalized();

		float4x4 rotation{ IDENTITY4 };

		rotation[0][0] = right[0];
		rotation[0][1] = correctedUp[0];
		rotation[0][2] = forward[0];

		rotation[1][0] = right[1];
		rotation[1][1] = correctedUp[1];
		rotation[1][2] = forward[1];

		rotation[2][0] = right[2];
		rotation[2][1] = correctedUp[2];
		rotation[2][2] = forward[2];

		transform.set_rotation(rotation);
	}
}
