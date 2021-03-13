
#include "scene.h"

#include "engine/debug/debug.h"

#include "utilities/math.h"

namespace al::engine
{
    Scene::Scene(EcsWorld* ecsWorld) noexcept
        : world{ ecsWorld }
        , root{ }
    {
        construct(&nodes);
        root = push(&nodes);
        al_assert(root);
        initialize_node_no_parent(root, EngineConfig::SCENE_ROOT_NODE_NAME);
    }

    Scene::~Scene() noexcept
    {
        for_each_array_container(nodes, it)
        {

        }
    }

    SceneNodeHandle Scene::create_node(std::string_view name) noexcept
    {
        SceneNodeHandle handle = push(&nodes);
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
            SceneNode* currentParent = scene_node(childNode->parent);
            for_each_array_container(currentParent->childs, it)
            {
                SceneNodeHandle* node = get(&currentParent->childs, it);
                if (*node == child)
                {
                    remove(&currentParent->childs, it);
                    break;
                }
            }
        }
        // Set new parent
        childNode->parent = parent;
        // Add to new parent
        if (parent != EMPTY_NODE_HANDLE)
        {
            push(&parentNode->childs, child);
        }
    }

    void Scene::update_transforms() noexcept
    {
        for_each_array_container(root->childs, it)
        {
            SceneNodeHandle* node = get(&root->childs, it);
            update_transform(*node);
        }
    }

    inline EcsWorld* Scene::get_ecs_world() noexcept
    {
        return world;
    }

    inline std::span<SceneNode> Scene::get_nodes() noexcept
    {
        return { nodes.memory, nodes.size };
    }

    void Scene::initialize_node_no_parent(SceneNodeHandle handle, std::string_view name) noexcept
    {
        al_assert(name.length() < EngineConfig::MAX_SCENE_NODE_NAME_LENGTH);
        SceneNode* node = scene_node(handle);
        new(node) SceneNode
        {
            .entityHandle       = ecs_create_entity(world),
            .scene              = this,
            .parent             = EMPTY_NODE_HANDLE
        };
        construct(&node->childs);
        construct(&node->name, name.data(), name.length());
        ecs_add_components<SceneTransform>(world, node->entityHandle);
        auto* sceneTransform = ecs_get_component<SceneTransform>(world, node->entityHandle);
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
        auto* parentSceneTransform = ecs_get_component<SceneTransform>(world, node->parent->entityHandle);
        auto* currentSceneTransform = ecs_get_component<SceneTransform>(world, node->entityHandle);
        currentSceneTransform->worldTransform.matrix = parentSceneTransform->get_world_transform().matrix * currentSceneTransform->get_local_transform().matrix;
        for_each_array_container(node->childs, it)
        {
            SceneNodeHandle child = *get(&node->childs, it);
            update_transform(child);
        }
    }
}
