
#include "scene_transform.h"

namespace al::engine
{
    SceneTransform::SceneTransform() noexcept
        : localTransform{ IDENTITY4 }
        , worldTransform{ IDENTITY4 }
    { }

    inline Transform SceneTransform::calculate_parent_transform() noexcept
    {
        return { worldTransform.matrix * localTransform.matrix.inverted() };
    }

    void SceneTransform::set_world_transform(const float4x4& trf) noexcept
    {
        Transform parentTrf = calculate_parent_transform();
        worldTransform.matrix = trf;
        localTransform.matrix = parentTrf.matrix.inverted() * worldTransform.matrix;
    }

    inline Transform SceneTransform::get_world_transform() noexcept
    {
        return worldTransform;
    }

    void SceneTransform::set_local_transform(const float4x4& trf) noexcept
    {
        Transform parentTrf = calculate_parent_transform();
        localTransform.matrix = trf;
        worldTransform.matrix = parentTrf.matrix * localTransform.matrix;
    }

    inline Transform SceneTransform::get_local_transform() noexcept
    {
        return localTransform;
    }

    void SceneTransform::set_world_transform_no_update_local_transform(const float4x4& trf) noexcept
    {
        worldTransform.matrix = trf;
    }
}
