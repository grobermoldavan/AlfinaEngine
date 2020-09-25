#ifndef __ALFINA_RENDER_CAMERA_H__
#define __ALFINA_RENDER_CAMERA_H__

#include "engine/math/math.h"

namespace al::engine
{
	class PerspectiveRenderCamera
	{
	public:
		PerspectiveRenderCamera();

		PerspectiveRenderCamera
		(
			Transform	_transform,
			float		_aspectRatio,
			float		_nearPlane,
			float		_farPlane,
			float		_fovDeg
		);

		Transform& get_transform();

		float4x4 get_projection() const;
		float4x4 get_view() const;

		void look_at(const float3& target, const float3& up);

	private:
		Transform	transform;
		float		aspectRatio;
		float		nearPlane;
		float		farPlane;
		float		fovDeg;
	};
}

#if defined(AL_UNITY_BUILD)
#	include "render_camera.cpp"
#else

#endif

#endif
