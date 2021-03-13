
#include "perspective_render_camera.h"

namespace al::engine
{
    PerspectiveRenderCamera::PerspectiveRenderCamera() noexcept
        : transform{ }
        , aspectRatio{ 4.0f / 3.0f }
        , nearPlane{ 0.1f }
        , farPlane{ 100.0f }
        , fovDeg{ 90.0f }
    { }

    PerspectiveRenderCamera::PerspectiveRenderCamera(Transform transform, float aspectRatio, float nearPlane, float farPlane, float fovDeg) noexcept
        : transform{ transform }
        , aspectRatio{ aspectRatio }
        , nearPlane{ nearPlane }
        , farPlane{ farPlane }
        , fovDeg{ fovDeg }
    { }

    Transform* PerspectiveRenderCamera::get_transform() noexcept
    {
        return &transform;
    }

    float4x4 PerspectiveRenderCamera::get_projection() const noexcept
    {
        float4x4 result{ };
        float tanHalfFov = std::tan(to_radians(fovDeg) / 2.0f);
        result.m[0][0] = 1.0f / (aspectRatio * tanHalfFov);
        result.m[1][1] = 1.0f / tanHalfFov;
        result.m[2][2] = 1.0f * (farPlane + nearPlane) / (farPlane - nearPlane);
        result.m[3][2] = 1.0f;
        result.m[2][3] = -1.0f * (2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
        return result;
    }

    float4x4 PerspectiveRenderCamera::get_view() const noexcept
    {
        return transform.matrix.inverted();
    }

    void PerspectiveRenderCamera::look_at(const float3& target, const float3& up) noexcept
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
        transform.set_rotation(rotation);
    }

    void PerspectiveRenderCamera::set_fov(float fov) noexcept
    {
        fovDeg = fov;
    }

    float PerspectiveRenderCamera::get_fov() const noexcept
    {
        return fovDeg;
    }

    void PerspectiveRenderCamera::set_aspect_ratio(float ratio) noexcept
    {
        aspectRatio = ratio;
    }

    float PerspectiveRenderCamera::get_aspect_ratio() const noexcept
    {
        return aspectRatio;
    }
}
