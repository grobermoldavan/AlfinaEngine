#ifndef AL_SCENE_TRANSFORM_H
#define AL_SCENE_TRANSFORM_H

#include <limits>

#include "engine/memory/memory_common.h"

#include "utilities/math.h"
#include "utilities/constexpr_functions.h"

namespace al::engine
{
    class al_align Transform
    {
    public:
        Transform() noexcept;
        Transform(const float4x4& trf) noexcept;
        ~Transform() = default;

        float4x4 get() noexcept;
        void set(const float4x4& other) noexcept;

        float3 get_forward  () const noexcept;
        float3 get_up       () const noexcept;
        float3 get_right    () const noexcept;
        float3 get_position () const noexcept;
        float3 get_scale    () const noexcept;
        float3 get_rotation () const noexcept;

        float3 get_inverted_scale() const noexcept;

        void set_position   (const float3& pos) noexcept;
        void add_position   (const float3& pos) noexcept;
        void set_scale      (const float3& scale) noexcept;
        void clear_scale    () noexcept;
        void add_scale      (const float3& scale) noexcept;
        void set_rotation   (const float3& eulerAngles) noexcept;

    private:
        float4x4 mat;
    };

    class al_align SceneTransform
    {
    public:
        SceneTransform() noexcept;
        ~SceneTransform() = default;

        void set_parent_transform(const float4x4& trf) noexcept;
        Transform get_parent_transform() noexcept;

        void set_world_transform(const float4x4& trf) noexcept;
        Transform get_world_transform() noexcept;

        void set_local_transform(const float4x4& trf) noexcept;
        Transform get_local_transform() noexcept;

    private:
        Transform localTransform;
        Transform worldTransform;
    };
}

#endif
