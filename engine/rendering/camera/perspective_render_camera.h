#ifndef AL_PERSPECTIVE_CAMERA_H
#define AL_PERSPECTIVE_CAMERA_H

#include "render_camera.h"

#include "utilities/math.h"

namespace al::engine
{
    class PerspectiveRenderCamera : public RenderCamera
    {
    public:
        PerspectiveRenderCamera() noexcept;
        PerspectiveRenderCamera(Transform transform, float aspectRatio, float nearPlane, float farPlane, float fovDeg) noexcept;
        ~PerspectiveRenderCamera() = default;

        virtual Transform* get_transform() noexcept override;
        virtual float4x4 get_projection() const noexcept override;
        virtual float4x4 get_view() const noexcept override;
        virtual void look_at(const float3& target, const float3& up) noexcept override;

        void set_fov(float fov) noexcept;
        float get_fov() const noexcept;
        void set_aspect_ratio(float ratio) noexcept;
        float get_aspect_ratio() const noexcept;

    private:
        Transform   transform;
        float       aspectRatio;
        float       nearPlane;
        float       farPlane;
        float       fovDeg;
    };
}

#endif
