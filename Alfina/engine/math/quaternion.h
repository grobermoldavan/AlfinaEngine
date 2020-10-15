#ifndef __ALFINA_QUATERNION_H__
#define __ALFINA_QUATERNION_H__

#include <cmath>

#include "math_types.h"
#include "utilities/constexpr_functions.h"

namespace al
{
	class Quaternion
	{
	public:
		struct AxisAngle
		{
			float3 axis;
			float angle;
		};

		Quaternion();
		Quaternion(const float3&, float);
		Quaternion(const Quaternion&);

		void		set_axis_angle(const float3& axis, float angle);
		AxisAngle	get_axis_angle() const;
		void		set_euler_angles(float3 angles);
		float3		get_euler_angles() const;

		Quaternion add(const Quaternion& other) const;
		Quaternion sub(const Quaternion& other) const;
		Quaternion mul(const Quaternion& other) const;
		Quaternion mul(const float scalar) const;
		Quaternion div(const Quaternion& other) const;
		Quaternion div(const float scalar) const;

		float		length_squared() const;
		float		length() const;
		void		normalize();
		Quaternion	normalized() const;
		void		invert();
		Quaternion	inverted() const;

	private:
		float3 imaginary;
		float real;
	};

	Quaternion::Quaternion()
		: imaginary{ 0, 0, 0 }
		, real{ 1 }
	{ }

	Quaternion::Quaternion(const float3& _imaginary, float _real)
		: imaginary{ _imaginary }
		, real{ _real }
	{ }

	Quaternion::Quaternion(const Quaternion& other)
		: imaginary{ other.imaginary }
		, real{ other.real }
	{ }

	void Quaternion::set_axis_angle(const float3& axis, float angle)
	{
		float radians = to_radians(angle);
		real = std::cos(radians / 2.0f);
		imaginary = axis * std::sin(radians / 2.0f);
	}

	Quaternion::AxisAngle Quaternion::get_axis_angle() const
	{
		float acosAngle = std::acos(real);
		float sinAngle = std::sin(acosAngle);

		if (is_equal(std::abs(sinAngle), 0.0f))
		{
			return
			{
				{ 0, 1, 0 },
				to_degrees(acosAngle * 2.0f)
			};
		}
		else
		{
			return
			{
				imaginary / sinAngle,
				to_degrees(acosAngle * 2.0f)
			};
		}
	}

	void Quaternion::set_euler_angles(float3 angles)
	{
		float cy = std::cos(to_radians(angles[2]) * 0.5f);
		float sy = std::sin(to_radians(angles[2]) * 0.5f);
		float cp = std::cos(to_radians(angles[1]) * 0.5f);
		float sp = std::sin(to_radians(angles[1]) * 0.5f);
		float cr = std::cos(to_radians(angles[0]) * 0.5f);
		float sr = std::sin(to_radians(angles[0]) * 0.5f);

		imaginary[0] = sr * cp * cy - cr * sp * sy;
		imaginary[1] = cr * sp * cy + sr * cp * sy;
		imaginary[2] = cr * cp * sy - sr * sp * cy;
		real = cr * cp * cy + sr * sp * sy;
	}

	float3 Quaternion::get_euler_angles() const
	{
		float test = imaginary[0] * imaginary[1] + imaginary[2] * real;
		if (test > 0.5f || is_equal(test, 0.5f))
		{
			return
			{
				0.0f,
				2.0f * to_degrees(std::atan2(imaginary[0], real)),
				90.0f
			};
		}
		else if (test < -0.5f || is_equal(test, -0.5f))
		{
			return
			{
				0.0f,
				-1.0f * 2.0f * to_degrees(std::atan2(imaginary[0], real)),
				-1.0f * 90.0f
			};
		}

		float3 result;

		float squareX = imaginary[0] * imaginary[0];
		float squareY = imaginary[1] * imaginary[1];
		float squareZ = imaginary[2] * imaginary[2];

		float denom = 1.0f - 2.0f * (squareY + squareZ);
		result[1] = to_degrees(std::atan2(2.0f * (imaginary[1] * real - imaginary[0] * imaginary[2]), denom));
		result[2] = to_degrees(std::asin(2.0f * test));

		denom = 1.0f - 2.0f * (squareX + squareZ);
		result[0] = to_degrees(std::atan2(2.0f * (imaginary[0] * real - imaginary[1] * imaginary[2]), denom));

		return result;
	}

	Quaternion Quaternion::add(const Quaternion& other) const
	{
		return
		{
			imaginary + other.imaginary,
			real + other.real
		};
	}

	Quaternion Quaternion::sub(const Quaternion& other) const
	{
		return
		{
			imaginary - other.imaginary,
			real - other.real
		};
	}

	Quaternion Quaternion::mul(const Quaternion& other) const
	{
		return
		{
			other.imaginary * real + imaginary * other.real + imaginary.cross(other.imaginary),
			real * other.real * imaginary.dot(other.imaginary)
		};
	}

	Quaternion Quaternion::mul(const float scalar) const
	{
		return
		{
			imaginary * scalar,
			real * scalar
		};
	}

	Quaternion Quaternion::div(const Quaternion& other) const
	{
		return mul(other.inverted());
	}

	Quaternion Quaternion::div(const float scalar) const
	{
		return
		{
			imaginary / scalar,
			real / scalar
		};
	}

	float Quaternion::length_squared() const
	{
		return real * real + imaginary.dot(imaginary);
	}

	float Quaternion::length() const
	{
		return std::sqrt(length_squared());
	}

	void Quaternion::normalize()
	{
		const float len = length();
		imaginary = imaginary / len;
		real = real / len;
	}

	Quaternion Quaternion::normalized() const
	{
		return div(length());
	}

	void Quaternion::invert()
	{
		imaginary = imaginary * -1.0f;
	}

	Quaternion Quaternion::inverted() const
	{
		return
		{
			imaginary * -1.0f,
			real
		};
	}
}

#endif
