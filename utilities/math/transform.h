#ifndef AL_TRANSFORM_H
#define AL_TRANSFORM_H

#include <cmath>

#include "vectors.h"
#include "matrices.h"
#include "quaternion.h"
#include "utilities/constexpr_functions.h"

namespace al
{
    struct Transform
    {
        float4x4 matrix = IDENTITY4;

        float3      get_position()          const noexcept;
        float3      get_rotation()          const noexcept;
        float3      get_scale()             const noexcept;
        float3      get_forward()           const noexcept;
        float3      get_right()             const noexcept;
        float3      get_up()                const noexcept;
        float3      get_inverted_scale()    const noexcept;

        void set_position       (const float3& position)    noexcept;
        void set_rotation       (const float3& eulerAngles) noexcept;
        void set_rotation       (const float4x4& rotation)  noexcept;
        void set_scale          (const float3& scale)       noexcept;
        void clear_scale        ()                          noexcept;
        void add_scale          (const float3& scale)       noexcept;
    };

    inline float3 Transform::get_position() const noexcept
    {
        return { matrix._03, matrix._13, matrix._23 };
    }

    inline float3 Transform::get_rotation() const noexcept
    {
        Quaternion quat;
        quat.set_mat(matrix);
        return quat.get_euler_angles();
    }

    inline float3 Transform::get_scale() const noexcept
    {
        return
        {
            float3{ matrix._00, matrix._01, matrix._02 }.len(),
            float3{ matrix._10, matrix._11, matrix._12 }.len(),
            float3{ matrix._20, matrix._21, matrix._22 }.len()
        };
    }

    inline float3 Transform::get_forward() const noexcept
    {
        return float3{ matrix.m[0][2], matrix.m[1][2], matrix.m[2][2] }.normalized();
    }

    inline float3 Transform::get_right() const noexcept
    {
        return float3{ matrix.m[0][0], matrix.m[1][0], matrix.m[2][0] }.normalized();
    }

    inline float3 Transform::get_up() const noexcept
    {
        return float3{ matrix.m[0][1], matrix.m[1][1], matrix.m[2][1] }.normalized();
    }

    inline float3 Transform::get_inverted_scale() const noexcept
    {
        float3 scale = get_scale();
        return
        {
            is_equal(scale.x, 0.0f) ?  (std::numeric_limits<float>::max)() : 1.0f / scale.x,
            is_equal(scale.y, 0.0f) ?  (std::numeric_limits<float>::max)() : 1.0f / scale.y,
            is_equal(scale.z, 0.0f) ?  (std::numeric_limits<float>::max)() : 1.0f / scale.z
        };
    }

    inline void Transform::set_position(const float3& position) noexcept
    {
        matrix._03 = position.x;
        matrix._13 = position.y;
        matrix._23 = position.z;
    }

    inline void Transform::set_rotation(const float3& eulerAngles) noexcept
    {
        float3 pos = get_position();
        float3 scale = get_scale();
        float4x4 tr = construct_translation_mat(pos);
        float4x4 rt = construct_rotation_mat(eulerAngles); 
        float4x4 sc = construct_scale_mat(scale);
        matrix = tr * rt * sc;
    }

    inline void Transform::set_rotation(const float4x4& rotation) noexcept
    {
        float3 pos = get_position();
        float3 scale = get_scale();
        float4x4 tr = construct_translation_mat(pos);
        float4x4 rt = rotation; 
        float4x4 sc = construct_scale_mat(scale);
        matrix = tr * rt * sc;
    }

    inline void Transform::set_scale(const float3& scale) noexcept
    {
        clear_scale();
        add_scale(scale);
    }

    inline void Transform::clear_scale() noexcept
    {
        add_scale(get_inverted_scale());
    }

    inline void Transform::add_scale(const float3& scale) noexcept
    {
        matrix._00 *= scale.x; matrix._01 *= scale.x; matrix._02 *= scale.x; // smdify?
        matrix._10 *= scale.y; matrix._11 *= scale.y; matrix._12 *= scale.y; // smdify?
        matrix._20 *= scale.z; matrix._21 *= scale.z; matrix._22 *= scale.z; // smdify?
    }
}

#endif
