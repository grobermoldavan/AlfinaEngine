#if defined(AL_UNITY_BUILD)

#else
#	include "transform.h"
#endif

#include <cmath>
#include <numbers>

#include "utilities/constexpr_functions.h"

namespace al::engine
{
	Transform::Transform()
		: translationMat{ IDENTITY4 }
		, rotationMat	{ IDENTITY4 }
		, scaleMat		{ IDENTITY4 }
	{ }

	Transform::Transform(const float3& position, const float3& euler, const float3& scale)
	{
		set_position(position);
		set_rotation(euler);
		set_scale(scale);
	}

	float4x4 Transform::get_matrix() const
	{
		return translationMat * rotationMat * scaleMat;
	}

	float3 Transform::get_position() const
	{
		return{ translationMat[0][3], translationMat[1][3], translationMat[2][3] };
	}

	float3 Transform::get_rotation() const
	{
		float3 result;

		if (is_equal(rotationMat[1][0], 1.0f))
		{
			result[0] = 0.0f;
			result[1] = std::atan2(rotationMat[0][2], rotationMat[2][2]);
			result[2] = std::numbers::pi_v<float> / 2.0f;
		}
		else if (is_equal(rotationMat[1][0], -1.0f))
		{
			result[0] = 0.0f;
			result[1] = std::atan2(rotationMat[0][2], rotationMat[2][2]);
			result[2] = -1.0f * std::numbers::pi_v<float> / 2.0f;
		}
		else
		{
			result[0] = std::atan2(-1.0f * rotationMat[1][2], rotationMat[1][1]);
			result[1] = std::atan2(-1.0f * rotationMat[2][0], rotationMat[0][0]);
			result[2] = std::asin(rotationMat[1][0]);
		}

		return
		{
			to_degrees(result[0]),
			to_degrees(result[1]),
			to_degrees(result[2])
		};
	}

	float3 Transform::get_scale() const
	{
		return{ scaleMat[0][0], scaleMat[1][1], scaleMat[2][2] };
	}

	float3 Transform::get_forward() const
	{
		return{ rotationMat[0][2], rotationMat[1][2], rotationMat[2][2] };
	}

	float3 Transform::get_right() const
	{
		return{ rotationMat[0][0], rotationMat[1][0], rotationMat[2][0] };
	}

	float3 Transform::get_up() const
	{
		return{ rotationMat[0][1], rotationMat[1][1], rotationMat[2][1] };
	}

	void Transform::set_position(const float3& position)
	{
		translationMat =
		{
			1, 0, 0, position[0],
			0, 1, 0, position[1],
			0, 0, 1, position[2],
			0, 0, 0, 1,
		};
	}

	void Transform::set_rotation(const float3& euler)
	{
		const float cosPitch = std::cos(to_radians(euler[0]));
		const float sinPitch = std::sin(to_radians(euler[0]));

		const float cosYaw = std::cos(to_radians(euler[1]));
		const float sinYaw = std::sin(to_radians(euler[1]));

		const float cosRoll = std::cos(to_radians(euler[2]));
		const float sinRoll = std::sin(to_radians(euler[2]));

#if 1
		const float4x4 pitch
		{
			1, 0       , 0               , 0,
			0, cosPitch, -1.0f * sinPitch, 0,
			0, sinPitch,         cosPitch, 0,
			0, 0       , 0               , 1
		};

		const float4x4 yaw
		{
			cosYaw        , 0, sinYaw, 0,
			0             , 1, 0     , 0,
			-1.0f * sinYaw, 0, cosYaw, 0,
			0             , 0, 0     , 1
		};

		const float4x4 roll
		{
			cosRoll, -1.0f * sinRoll, 0, 0,
			sinRoll,         cosRoll, 0, 0,
			0      , 0              , 1, 0,
			0      , 0              , 0, 1
		};

		rotationMat = yaw * pitch * roll;
#else
		rotationMat[0][0] = cosYaw * cosRoll + sinYaw * sinPitch * sinRoll;
		rotationMat[0][1] = cosRoll * sinYaw * sinPitch - sinRoll * cosYaw;
		rotationMat[0][2] = cosPitch * sinYaw;

		rotationMat[1][0] = cosPitch * sinRoll;
		rotationMat[1][1] = cosRoll * cosPitch;
		rotationMat[1][2] = -1.0f * sinPitch;

		rotationMat[2][0] = sinRoll * cosYaw * sinPitch - sinYaw * cosRoll;
		rotationMat[2][1] = sinYaw * sinRoll + cosRoll * cosYaw * sinPitch;
		rotationMat[2][2] = cosPitch * cosYaw;
#endif
	}

	void Transform::set_scale(const float3& scale)
	{
		scaleMat =
		{
			scale[0], 0       , 0       , 0,
			0       , scale[1], 0       , 0,
			0       , 0       , scale[2], 0,
			0       , 0       , 0       , 1
		};
	}

	void Transform::set_position_mat(const float4x4& position)
	{
		translationMat = position;
	}

	void Transform::set_rotation_mat(const float4x4& rotation)
	{
		rotationMat = rotation;
	}

	void Transform::set_scale_mat(const float4x4& scale)
	{
		scaleMat = scale;
	}
}
