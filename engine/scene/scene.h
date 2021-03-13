#ifndef AL_SCENE_H
#define AL_SCENE_H

#include <string_view>
#include <span>

#include "scene_transform.h"
#include "engine/config/engine_config.h"
#include "engine/memory/memory_common.h"
#include "engine/ecs/ecs.h"
#include "engine/containers/containers.h"

#include "utilities/array_container.h"
#include "utilities/function.h"
#include "utilities/fixed_size_string.h"

namespace al::engine
{
    class Scene;
    struct SceneNode;

    using SceneSizeT = uint64_t;
    using SceneNodeHandle = SceneNode*;
    using SceneNodeName = FixedSizeString<EngineConfig::MAX_SCENE_NODE_NAME_LENGTH>;

    struct al_align SceneNode
    {
        using ChildContainer = ArrayContainer<SceneNodeHandle, EngineConfig::MAX_NUMBER_OF_NODE_CHILDS>;
        EcsEntityHandle entityHandle;
        Scene*          scene;
        SceneNodeHandle parent;
        ChildContainer  childs;
        SceneNodeName   name;
    };

    class Scene
    {
    public:
        static constexpr SceneNodeHandle EMPTY_NODE_HANDLE = nullptr;

        Scene(EcsWorld* ecsWorld) noexcept;
        ~Scene() noexcept;

        SceneNodeHandle create_node(std::string_view name = "SceneNode") noexcept;
        SceneNode* scene_node(SceneNodeHandle handle) noexcept;

        void set_parent(SceneNodeHandle child, SceneNodeHandle parent) noexcept;
        void update_transforms() noexcept;

        EcsWorld* get_ecs_world() noexcept;
        std::span<SceneNode> get_nodes() noexcept;

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
