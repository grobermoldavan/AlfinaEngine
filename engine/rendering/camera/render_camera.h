#ifndef AL_CAMERA_H
#define AL_CAMERA_H

#include "utilities/math.h"

namespace al::engine
{
    class RenderCamera
    {
    public:
        virtual ~RenderCamera() = default;

        virtual Transform& get_transform() noexcept = 0;
        virtual float4x4 get_projection() const noexcept = 0;
        virtual float4x4 get_view() const noexcept = 0;
        virtual void look_at(const float3& target, const float3& up) noexcept = 0;
    };
}

#endif
