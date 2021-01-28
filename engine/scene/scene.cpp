
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
        initialize_node_no_parent(root, "RootNode");
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
        parentNode->childs.push(child);
    }

    void Scene::update_transforms() noexcept
    {
        for (SceneNodeHandle rootChild : root->childs)
        {
            update_transform(rootChild);
        }
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
        // // Each scene node has world transform
        // world->add_components<SceneWorldTransform>(node->entityHandle);
        // auto* worldTrf = world->get_component<SceneWorldTransform>(node->entityHandle);
        // new(worldTrf) SceneWorldTransform{ };
        // // Each scene node has local transform
        // world->add_components<SceneLocalTransform>(node->entityHandle);
        // auto* localTrf = world->get_component<SceneLocalTransform>(node->entityHandle);
        // new(localTrf) SceneLocalTransform{  };
    }

    void Scene::initialize_node(SceneNodeHandle handle, SceneNodeHandle parent, std::string_view name) noexcept
    {
        initialize_node_no_parent(handle, name);
        set_parent(handle, parent);
    }

    void Scene::update_transform(SceneNodeHandle handle) noexcept
    {
        SceneNode* node = scene_node(handle);
        // auto* parentWorldTransform = world->get_component<SceneWorldTransform>(node->parent->entityHandle);
        // auto* currentWorldTransform = world->get_component<SceneWorldTransform>(node->entityHandle);
        // auto* currentLocalTransform = world->get_component<SceneLocalTransform>(node->entityHandle);
        // currentWorldTransform->trf.set_full_transform(parentWorldTransform->trf.get_full_transform() * currentLocalTransform->trf.get_full_transform());
        // for (SceneNodeHandle child : node->childs)
        // {
        //     update_transform(child);
        // }
    }
}
