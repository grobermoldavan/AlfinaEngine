#ifndef AL_PERSPECTIVE_CAMERA_H
#define AL_PERSPECTIVE_CAMERA_H

#include "render_camera.h"

#include "utilities/math.h"

namespace al::engine
{
    class PerspectiveRenderCamera : public RenderCamera
    {
    public:
        PerspectiveRenderCamera() noexcept
            : transform{ }
            , aspectRatio{ 4.0f / 3.0f }
            , nearPlane{ 0.1f }
            , farPlane{ 100.0f }
            , fovDeg{ 90.0f }
        { }

        PerspectiveRenderCamera(Transform transform, float aspectRatio, float nearPlane, float farPlane, float fovDeg) noexcept
            : transform{ transform }
            , aspectRatio{ aspectRatio }
            , nearPlane{ nearPlane }
            , farPlane{ farPlane }
            , fovDeg{ fovDeg }
        { }

        ~PerspectiveRenderCamera() = default;

        virtual Transform& get_transform() noexcept override
        {
            return transform;
        }

        virtual float4x4 get_projection() const noexcept override
        {
            float4x4 result{ 0 };
            float tanHalfFov = std::tan(to_radians(fovDeg) / 2.0f);
            result.m[0][0] = 1.0f / (aspectRatio * tanHalfFov);
            result.m[1][1] = 1.0f / tanHalfFov;
            result.m[2][2] = 1.0f * (farPlane + nearPlane) / (farPlane - nearPlane);
            result.m[3][2] = 1.0f;
            result.m[2][3] = -1.0f * (2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
            return result;
        }

        virtual float4x4 get_view() const noexcept override
        {
            return transform.get_full_transform().inverted();
        }

        virtual void look_at(const float3& target, const float3& up) noexcept override
        {
            float3 forward      = (target - transform.get_position()).normalized();
            float3 right        = forward.cross(up).normalized();
            float3 correctedUp  = right.cross(forward).normalized();

            float4x4 rotation{ IDENTITY4 };

            rotation.m[0][0] = right.x;
            rotation.m[0][1] = correctedUp.x;
            rotation.m[0][2] = forward.x;

            rotation.m[1][0] = right.y;
            rotation.m[1][1] = correctedUp.y;
            rotation.m[1][2] = forward.y;

            rotation.m[2][0] = right.z;
            rotation.m[2][1] = correctedUp.z;
            rotation.m[2][2] = forward.z;

            transform.rotation = rotation;
        }

    private:
        Transform   transform;
        float       aspectRatio;
        float       nearPlane;
        float       farPlane;
        float       fovDeg;
    };
}

#endif
