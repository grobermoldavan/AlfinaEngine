#ifndef AL_SCENE_H
#define AL_SCENE_H

#include <string_view>

#include "scene_transform.h"
#include "engine/config/engine_config.h"
#include "engine/memory/memory_common.h"
#include "engine/ecs/ecs.h"
#include "engine/containers/containers.h"

#include "utilities/array_container.h"
#include "utilities/function.h"

namespace al::engine
{
    class Scene;
    struct SceneNode;

    using SceneSizeT = uint64_t;
    using SceneNodeHandle = SceneNode*;

    struct al_align SceneNode
    {
        using ChildContainer = ArrayContainer<SceneNodeHandle, EngineConfig::MAX_NUMBER_OF_NODE_CHILDS>;
        EntityHandle    entityHandle;
        Scene*          scene;
        SceneNodeHandle parent;
        ChildContainer  childs;
        char            name[EngineConfig::MAX_SCENE_NODE_NAME_LENGTH];
    };

    class Scene
    {
    private:
        static constexpr SceneNodeHandle EMPTY_NODE_HANDLE = nullptr;

    public:
        Scene(EcsWorld* ecsWorld) noexcept;
        ~Scene() noexcept;

        SceneNodeHandle create_node(std::string_view name = "SceneNode") noexcept;
        SceneNode* scene_node(SceneNodeHandle handle) noexcept;

        void set_parent(SceneNodeHandle child, SceneNodeHandle parent) noexcept;
        void update_transforms() noexcept;

        EcsWorld* get_ecs_world() noexcept;

    private:
        EcsWorld* world;
        SceneNodeHandle root;
        ArrayContainer<SceneNode, EngineConfig::MAX_NUMBER_OF_SCENE_NODES> nodes;

        void initialize_node_no_parent(SceneNodeHandle handle, std::string_view name) noexcept;
        void initialize_node(SceneNodeHandle handle, SceneNodeHandle parent, std::string_view name) noexcept;
        void update_transform(SceneNodeHandle handle) noexcept;
    };
}

#endif
