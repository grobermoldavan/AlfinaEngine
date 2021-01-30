
#include "scene.h"

#include "engine/debug/debug.h"

#include "utilities/math.h"

namespace al::engine
{
    Scene::Scene(EcsWorld* ecsWorld) noexcept
        : world{ ecsWorld }
        , root{ }
        , nodes{ }
    {
        root = nodes.get();
        al_assert(root);
        initialize_node_no_parent(root, EngineConfig::SCENE_ROOT_NODE_NAME);
    }

    Scene::~Scene() noexcept
    {
        nodes.for_each([&](SceneNode* node)
        {
            // @TODO :  remove entity from ecs world
        });
    }

    SceneNodeHandle Scene::create_node(std::string_view name) noexcept
    {
        SceneNodeHandle handle = nodes.get();
        al_assert(handle);
        initialize_node(handle, root, name);
        return handle;
    }

    inline SceneNode* Scene::scene_node(SceneNodeHandle handle) noexcept
    {
        return handle;
    }

    void Scene::set_parent(SceneNodeHandle child, SceneNodeHandle parent) noexcept
    {
        SceneNode* childNode = scene_node(child);
        SceneNode* parentNode = scene_node(parent);
        // Remove from parent
        if (childNode->parent != EMPTY_NODE_HANDLE)
        {
            scene_node(childNode->parent)->childs.remove_by_condition([child](SceneNodeHandle* node) -> bool
            {
                return *node == child;
            });
        }
        // Set new parent
        childNode->parent = parent;
        // Add to new parent
        if (parent != EMPTY_NODE_HANDLE)
        {
            parentNode->childs.push(child);
        }
    }

    void Scene::update_transforms() noexcept
    {
        for (SceneNodeHandle rootChild : root->childs)
        {
            update_transform(rootChild);
        }
    }

    inline EcsWorld* Scene::get_ecs_world() noexcept
    {
        return world;
    }

    inline std::span<SceneNode> Scene::get_nodes() noexcept
    {
        return { nodes.data(), nodes.get_current_size() };
    }

    void Scene::initialize_node_no_parent(SceneNodeHandle handle, std::string_view name) noexcept
    {
        al_assert(name.length() < EngineConfig::MAX_SCENE_NODE_NAME_LENGTH);
        SceneNode* node = scene_node(handle);
        new(node) SceneNode
        {
            .entityHandle       = world->create_entity(),
            .scene              = this,
            .parent             = EMPTY_NODE_HANDLE,
            .childs             = { },
            .name               = { 0 }
        };
        std::memcpy(node->name, name.data(), name.length());
        world->add_components<SceneTransform>(node->entityHandle);
        auto* sceneTransform = world->get_component<SceneTransform>(node->entityHandle);
        new(sceneTransform) SceneTransform{ };
    }

    void Scene::initialize_node(SceneNodeHandle handle, SceneNodeHandle parent, std::string_view name) noexcept
    {
        initialize_node_no_parent(handle, name);
        set_parent(handle, parent);
    }

    void Scene::update_transform(SceneNodeHandle handle) noexcept
    {
        SceneNode* node = scene_node(handle);
        auto* parentSceneTransform = world->get_component<SceneTransform>(node->parent->entityHandle);
        auto* currentSceneTransform = world->get_component<SceneTransform>(node->entityHandle);
        currentSceneTransform->worldTransform.matrix = parentSceneTransform->get_world_transform().matrix * currentSceneTransform->get_local_transform().matrix;
        for (SceneNodeHandle child : node->childs)
        {
            update_transform(child);
        }
    }
}
