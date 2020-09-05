#ifndef __ALFINA_QUATERNION_H__
#define __ALFINA_QUATERNION_H__

#include "math_types.h"

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

	const Quaternion IDENTITY_QUAT;
}

#if defined(AL_UNITY_BUILD)
#	include "quaternion.cpp"
#else

#endif

#endif
