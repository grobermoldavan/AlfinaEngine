#ifndef AL_TRANSFORM_H
#define AL_TRANSFORM_H

#include <cmath>

#include "vectors.h"
#include "matrices.h"
#include "utilities/constexpr_functions.h"

namespace al
{
    struct deprecated_Transform
    {
        float4x4 translation    = IDENTITY4;
        float4x4 rotation       = IDENTITY4;
        float4x4 scale          = IDENTITY4;

        float4x4    get_full_transform()    const noexcept;
        float3      get_position()          const noexcept;
        float3      get_rotation()          const noexcept;
        float3      get_scale()             const noexcept;
        float3      get_forward()           const noexcept;
        float3      get_right()             const noexcept;
        float3      get_up()                const noexcept;

        void set_position       (const float3& position)    noexcept;
        void set_rotation       (const float3& eulerAngles) noexcept;
        void set_scale          (const float3& vec)         noexcept;
        void set_full_transform (const float4x4& trf)       noexcept;
    };

    inline float4x4 deprecated_Transform::get_full_transform() const noexcept
    {
        return translation * rotation * scale;
    }

    inline float3 deprecated_Transform::get_position() const noexcept
    {
        return { translation.m[0][3], translation.m[1][3], translation.m[2][3] };
    }

    float3 deprecated_Transform::get_rotation() const noexcept
    {
        float3 result;

        if (is_equal(rotation.m[1][0], 1.0f))
        {
            result.x = 0.0f;
            result.y = std::atan2(rotation.m[0][2], rotation.m[2][2]);
            result.z = std::numbers::pi_v<float> / 2.0f;
        }
        else if (is_equal(rotation.m[1][0], -1.0f))
        {
            result.x = 0.0f;
            result.y = std::atan2(rotation.m[0][2], rotation.m[2][2]);
            result.z = -1.0f * std::numbers::pi_v<float> / 2.0f;
        }
        else
        {
            result.x = std::atan2(-1.0f * rotation.m[1][2], rotation.m[1][1]);
            result.y = std::atan2(-1.0f * rotation.m[2][0], rotation.m[0][0]);
            result.z = std::asin(rotation.m[1][0]);
        }

        return
        {
            to_degrees(result.x),
            to_degrees(result.y),
            to_degrees(result.z)
        };
    }

    inline float3 deprecated_Transform::get_scale() const noexcept
    {
        return{ scale.m[0][0], scale.m[1][1], scale.m[2][2] };
    }

    inline float3 deprecated_Transform::get_forward() const noexcept
    {
        return{ rotation.m[0][2], rotation.m[1][2], rotation.m[2][2] };
    }

    inline float3 deprecated_Transform::get_right() const noexcept
    {
        return{ rotation.m[0][0], rotation.m[1][0], rotation.m[2][0] };
    }

    inline float3 deprecated_Transform::get_up() const noexcept
    {
        return{ rotation.m[0][1], rotation.m[1][1], rotation.m[2][1] };
    }

    inline void deprecated_Transform::set_position(const float3& position) noexcept
    {
        translation.set(construct_translation_mat(position));
    }

    inline void deprecated_Transform::set_rotation(const float3& eulerAngles) noexcept
    {
        rotation = construct_rotation_mat(eulerAngles);
    }

    inline void deprecated_Transform::set_scale(const float3& vec) noexcept
    {
        scale.set(construct_scale_mat(vec));
    }

    void deprecated_Transform::set_full_transform (const float4x4& trf) noexcept
    {
        set_position({ trf._03, trf._13, trf._23 });
        float3 scale
        {
            float3{ trf._00, trf._10, trf._20 }.len(),
            float3{ trf._01, trf._11, trf._21 }.len(),
            float3{ trf._02, trf._12, trf._22 }.len()
        };
        set_scale(scale);
        rotation = float4x4
        {
            trf._00 / scale.x, trf._01 / scale.y, trf._02 / scale.z, 0.0f,
            trf._10 / scale.x, trf._11 / scale.y, trf._12 / scale.z, 0.0f,
            trf._20 / scale.x, trf._21 / scale.y, trf._22 / scale.z, 0.0f,
            0.0f             , 0.0f             , 0.0f             , 1.0f,
        };
    }
}

#endif
