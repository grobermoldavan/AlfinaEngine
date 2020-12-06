#ifndef AL_TRANSFORM_H
#define AL_TRANSFORM_H

#include <cmath>

#include "vectors.h"
#include "matrices.h"
#include "utilities/constexpr_functions.h"

namespace al
{
    float4x4 construct_translation_mat(const float3& position) noexcept
    {
        return
        {
            1, 0, 0, position.x,
            0, 1, 0, position.y,
            0, 0, 1, position.z,
            0, 0, 0, 1,
        };
    }

    float4x4 construct_rotation_mat(const float3& eulerAngles) noexcept
    {
        const float cosPitch = std::cos(to_radians(eulerAngles.x));
        const float sinPitch = std::sin(to_radians(eulerAngles.x));

        const float cosYaw = std::cos(to_radians(eulerAngles.y));
        const float sinYaw = std::sin(to_radians(eulerAngles.y));

        const float cosRoll = std::cos(to_radians(eulerAngles.z));
        const float sinRoll = std::sin(to_radians(eulerAngles.z));

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

        return yaw * pitch * roll;
    }

    float4x4 construct_scale_mat(const float3& scale) noexcept
    {
        return
        {
            scale.x , 0         , 0         , 0,
            0       , scale.y   , 0         , 0,
            0       , 0         , scale.z   , 0,
            0       , 0         , 0         , 1
        };
    }

    struct Transform
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

        void set_position   (const float3& position)    noexcept;
        void set_rotation   (const float3& eulerAngles) noexcept;
        void set_scale      (const float3& vec)         noexcept;
    };

    float4x4 Transform::get_full_transform() const noexcept
    {
        return translation * rotation * scale;
    }

    float3 Transform::get_position() const noexcept
    {
        return { translation.m[0][3], translation.m[1][3], translation.m[2][3] };
    }

    float3 Transform::get_rotation() const noexcept
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

    float3 Transform::get_scale() const noexcept
    {
        return{ scale.m[0][0], scale.m[1][1], scale.m[2][2] };
    }

    float3 Transform::get_forward() const noexcept
    {
        return{ rotation.m[0][2], rotation.m[1][2], rotation.m[2][2] };
    }

    float3 Transform::get_right() const noexcept
    {
        return{ rotation.m[0][0], rotation.m[1][0], rotation.m[2][0] };
    }

    float3 Transform::get_up() const noexcept
    {
        return{ rotation.m[0][1], rotation.m[1][1], rotation.m[2][1] };
    }

    void Transform::set_position(const float3& position) noexcept
    {
        translation.set(construct_translation_mat(position));
    }

    void Transform::set_rotation(const float3& eulerAngles) noexcept
    {
        rotation = construct_rotation_mat(eulerAngles);
    }

    void Transform::set_scale(const float3& vec) noexcept
    {
        scale.set(construct_scale_mat(vec));
    }
}

#endif
