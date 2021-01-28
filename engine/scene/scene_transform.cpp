
#include "scene_transform.h"

namespace al::engine
{
    Transform::Transform() noexcept
        : mat{ IDENTITY4 }
    { }

    Transform::Transform(const float4x4& trf) noexcept
        : mat{ trf }
    { }

    inline float4x4 Transform::get() noexcept
    {
        return mat;
    }

    void Transform::set(const float4x4& other) noexcept
    {
        mat = other;
    }

    inline float3 Transform::get_forward() const noexcept
    {
        return float3{ mat.m[0][2], mat.m[1][2], mat.m[2][2] }.normalized();
    }

    inline float3 Transform::get_up() const noexcept
    {
        return float3{ mat.m[0][1], mat.m[1][1], mat.m[2][1] }.normalized();
    }

    inline float3 Transform::get_right() const noexcept
    {
        return float3{ mat.m[0][0], mat.m[1][0], mat.m[2][0] }.normalized();
    }

    inline float3 Transform::get_position() const noexcept
    {
        return { mat._03, mat._13, mat._23 };
    }

    inline float3 Transform::get_scale() const noexcept
    {
        return
        {
            float3{ mat._00, mat._01, mat._02 }.len(),
            float3{ mat._10, mat._11, mat._12 }.len(),
            float3{ mat._20, mat._21, mat._22 }.len()
        };
    }

    inline float3 Transform::get_rotation() const noexcept
    {
        Quaternion quat;
        quat.set_mat(mat);
        return quat.get_euler_angles();
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

    inline void Transform::set_position(const float3& pos) noexcept
    {
        mat._03 = pos.x;
        mat._13 = pos.y;
        mat._23 = pos.z;
    }

    inline void Transform::add_position(const float3& pos) noexcept
    {
        mat._03 += pos.x;
        mat._13 += pos.y;
        mat._23 += pos.z;
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
        mat._00 *= scale.x; mat._01 *= scale.x; mat._02 *= scale.x; // smdify?
        mat._10 *= scale.y; mat._11 *= scale.y; mat._12 *= scale.y; // smdify?
        mat._20 *= scale.z; mat._21 *= scale.z; mat._22 *= scale.z; // smdify?
    }

    inline void Transform::set_rotation(const float3& eulerAngles) noexcept
    {
        Quaternion rot;
        rot.set_euler_angles(eulerAngles);
        float3 pos = get_position();
        float3 scale = get_scale();
        float4x4 tr = construct_translation_mat(pos);
        float4x4 rt = construct_rotation_mat(rot.get_euler_angles()); 
        float4x4 sc = construct_scale_mat(scale);
        mat = tr * rt * sc;
    }

    SceneTransform::SceneTransform() noexcept
        : localTransform{ }
        , worldTransform{ }
    { }

    inline void SceneTransform::set_parent_transform(const float4x4& trf) noexcept
    {
        worldTransform.set(trf * localTransform.get());
    }

    inline Transform SceneTransform::get_parent_transform() noexcept
    {
        return { worldTransform.get() * localTransform.get().inverted() };
    }

    void SceneTransform::set_world_transform(const float4x4& trf) noexcept
    {
        Transform parentTransform = get_parent_transform();
        worldTransform = parentTransform;
        localTransform.set(parentTransform.get().inverted() * worldTransform.get());
    }

    inline Transform SceneTransform::get_world_transform() noexcept
    {
        return worldTransform;
    }

    void SceneTransform::set_local_transform(const float4x4& trf) noexcept
    {
        Transform parentTransform = get_parent_transform();
        localTransform = trf;
        worldTransform = parentTransform.get() * localTransform.get();
    }

    inline Transform SceneTransform::get_local_transform() noexcept
    {
        return localTransform;
    }
}
