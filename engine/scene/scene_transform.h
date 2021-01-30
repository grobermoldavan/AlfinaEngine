#ifndef AL_SCENE_TRANSFORM_H
#define AL_SCENE_TRANSFORM_H

#include <limits>

#include "engine/memory/memory_common.h"

#include "utilities/math.h"
#include "utilities/constexpr_functions.h"

namespace al::engine
{
    class al_align SceneTransform
    {
    public:
        SceneTransform() noexcept;
        ~SceneTransform() = default;

        Transform calculate_parent_transform() noexcept;

        void set_world_transform(const float4x4& trf) noexcept;
        Transform get_world_transform() noexcept;

        void set_local_transform(const float4x4& trf) noexcept;
        Transform get_local_transform() noexcept;

        void set_world_transform_no_update_local_transform(const float4x4& trf) noexcept;

    private:
        Transform localTransform;
        Transform worldTransform;

        friend class Scene;
    };
}

#endif
