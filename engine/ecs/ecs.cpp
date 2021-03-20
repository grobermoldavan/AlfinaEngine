
#include <inttypes.h>

#include "ecs.h"

#include "engine/memory/memory_manager.h"
#include "engine/debug/debug.h"

#include "utilities/constexpr_functions.h"
#include "utilities/memory.h"
#include "utilities/procedural_wrap.h"

namespace al::engine
{
    // @NOTE :  Component count starts with number one, not zero.
    //          This is done because ECS system thinks that component array
    //          with componentId == 0 is not used (see ecs_is_valid_component_array).
    //          We could use additional bool value (isValid or something), but
    //          currently this is not very neccessary.
    EcsComponentId              gEcsComponentCount = 1;
    EcsComponentRuntimeInfo     gEcsComponentInfos[ECS_WORLD_MAX_COMPONENTS] = { };

    void construct(EcsWorld* world)
    {
        al_memzero(&world->entities);
        al_memzero(&world->archetypes);
        for (EcsSizeT it = 0; it < EngineConfig::ECS_MAX_ARCHETYPES; it++)
        {
            // @NOTE :  Set each archetype to the correct areas of the pools
            EcsArchetype* archetype = get(&world->archetypes, it);
            construct(&archetype->componentArrayPointers, &world->componentArrayPointersPool[it * ECS_WORLD_MAX_COMPONENTS]);
            construct(&archetype->entityHandles         , &world->entityHandlesPool[it * EngineConfig::ECS_MAX_ENTITIES_IN_ARCHETYPE_CHUNK]);
            construct(&archetype->chunks);
            archetype->selfHandle = it;
        }
        // @NOTE :  Setup first empty archetype
        EcsArchetype* archetype = push(&world->archetypes);
        archetype->componentFlags   = { };
        archetype->size             = 0;
        archetype->capacity         = 0;
        archetype->selfHandle       = 0;
    }

    void destruct(EcsWorld* world)
    {
        for_each_array_container(world->archetypes, archetypeIt)
        {
            EcsArchetype* archetype = get(&world->archetypes, archetypeIt);
            for_each_dynamic_array(archetype->chunks, chunkIt)
            {
                uint8_t* chunk = *get(&archetype->chunks, chunkIt);
                deallocate(&gMemoryManager->pool, chunk, EngineConfig::ECS_COMPONENT_ARRAY_CHUNK_SIZE);
            }
            destruct(&archetype->chunks);
        }
    }

    EcsEntityHandle ecs_create_entity(EcsWorld* world)
    {
        EcsEntityHandle handle = world->entities.size;
        bool pushResult = push(&world->entities,
        {
            .componentFlags = { },
            .archetypeHandle = 0,
            .archetypeArrayIndex = 0
        });
        al_assert_msg(pushResult, "Can't create new entity : pool is empty. Consider increasing EngineConfig::ECS_MAX_ENTITIES value.")
        return handle;
    }

    template<typename ... T>
    void ecs_add_components (EcsWorld* world, EcsEntityHandle handle)
    {
        ecs_register_components_if_needed<T...>();
        EcsEntity* entity = get(&world->entities, handle);
        EcsComponentFlags currentFlags = entity->componentFlags;
        EcsComponentFlags newFlags = entity->componentFlags;
        ecs_set_component_flags<T...>(&newFlags);
        if (currentFlags == newFlags)
        {
            return;
        }
        entity->componentFlags = newFlags;
        EcsArchetypeHandle oldArchetype = entity->archetypeHandle;
        EcsArchetypeHandle newArchetype = ecs_match_or_create_archetype(world, handle);
        ecs_move_entity_superset(world, oldArchetype, newArchetype, handle);
    }

    template<typename ... T>
    void ecs_remove_components(EcsWorld* world, EcsEntityHandle handle)
    {
        ecs_register_components_if_needed<T...>();
        EcsEntity* entity = get(&world->entities, handle);
        EcsComponentFlags currentFlags = entity->componentFlags;
        EcsComponentFlags newFlags = entity->componentFlags;
        ecs_clear_component_flags<T...>(&newFlags);
        if (currentFlags == newFlags)
        {
            return;
        }
        entity->componentFlags = newFlags;
        EcsArchetypeHandle oldArchetype = entity->archetypeHandle;
        EcsArchetypeHandle newArchetype = ecs_match_or_create_archetype(world, handle);
        ecs_move_entity_subset(world, oldArchetype, newArchetype, handle);
    }

    template<typename T>
    T* ecs_get_component(EcsWorld* world, EcsEntityHandle handle)
    {
        EcsEntity* entity = get(&world->entities, handle);
        return reinterpret_cast<T*>(ecs_access_component(world, entity->archetypeHandle, ecs_component_type_info_get_id<T>(), entity->archetypeArrayIndex));
    }

    template<typename ... T>
    void ecs_for_each_fp(EcsWorld* world, EcsForEachFunctionPointer<T...> func)
    {
        EcsComponentFlags requestFlags{ };
        ecs_set_component_flags<T...>(&requestFlags);
        for_each_array_container(world->archetypes, it)
        {
            EcsArchetype* archetype = get(&world->archetypes, it);
            if (!ecs_is_valid_subset(requestFlags, archetype->componentFlags))
            {
                continue;
            }
            for (EcsSizeT entityIt = 0; entityIt < archetype->size; entityIt++)
            {
                func(world, *get(&archetype->entityHandles, entityIt), ecs_access_component<T>(archetype, entityIt)...);
            }
        }
    }

    template<typename ... T>
    void ecs_for_each(EcsWorld* world, EcsForEachFunctionObject<T...> func)
    {
        EcsComponentFlags requestFlags{ };
        ecs_set_component_flags<T...>(&requestFlags);
        for_each_dynamic_array(world->archetypes, it)
        {
            EcsArchetype* archetype = get(&world->archetypes, it);
            if (!ecs_is_valid_subset(requestFlags, archetype->componentFlags))
            {
                continue;
            }
            for (EcsSizeT entityIt = 0; entityIt < archetype->size; entityIt++)
            {
                func(world, *get(&archetype->entityHandles, entityIt), ecs_access_component<T>(archetype, entityIt)...);
            }
        }
    }

    EcsArchetypeHandle ecs_match_or_create_archetype(EcsWorld* world, EcsEntityHandle handle)
    {
        EcsEntity* entity = get(&world->entities, handle);
        for_each_array_container(world->archetypes, it)
        {
            EcsArchetype* archetype = get(&world->archetypes, it);
            if (archetype->componentFlags == entity->componentFlags)
            {
                return archetype->selfHandle;
            }
        }
        return ecs_create_archetype(world, entity->componentFlags);
    }

    EcsArchetypeHandle ecs_create_archetype(EcsWorld* world, EcsComponentFlags flags)
    {
        EcsArchetype* archetype = push(&world->archetypes);
        al_assert_msg(archetype, "Can't create new archetype : pool is empty. Consider increasing EngineConfig::ECS_MAX_ARCHETYPES value.");
        archetype->componentFlags   = flags;
        archetype->size             = 0;
        archetype->capacity         = 0;
        archetype->selfHandle       = index_of(&world->archetypes, archetype);
        EcsSizeT singleEntrySize = 0;
        for (EcsSizeT it = 0; it < ECS_WORLD_MAX_COMPONENTS; it++)
        {
            if (!archetype->componentFlags.get_flag(it))
            {
                continue;
            }
            singleEntrySize += gEcsComponentInfos[it].sizeBytes;
        }
        archetype->singleChunkCapacity = EngineConfig::ECS_COMPONENT_ARRAY_CHUNK_SIZE / singleEntrySize;
        EcsSizeT currentOffset = 0;
        for (EcsSizeT it = 0; it < ECS_WORLD_MAX_COMPONENTS; it++)
        {
            if (!archetype->componentFlags.get_flag(it))
            {
                continue;
            }
            *get(&archetype->componentArrayPointers, it) = currentOffset;
            currentOffset += gEcsComponentInfos[it].sizeBytes * archetype->singleChunkCapacity;
        }
        return archetype->selfHandle;
    }

    void ecs_allocate_chunks(EcsWorld* world, EcsArchetypeHandle handle)
    {
        EcsArchetype* archetype = get(&world->archetypes, handle);
        push(&archetype->chunks, static_cast<uint8_t*>(allocate(&gMemoryManager->pool, EngineConfig::ECS_COMPONENT_ARRAY_CHUNK_SIZE)));
        archetype->capacity += archetype->singleChunkCapacity;
    }

    void ecs_move_entity_superset(EcsWorld* world, EcsArchetypeHandle from, EcsArchetypeHandle to, EcsEntityHandle handle)
    {
        EcsArchetype*   fromArchetype   = get(&world->archetypes, from);
        EcsArchetype*   toArchetype     = get(&world->archetypes, to);
        EcsEntity*      entityPtr       = get(&world->entities, handle);
        EcsSizeT        fromIndex       = entityPtr->archetypeArrayIndex;
        EcsSizeT        toIndex         = ecs_reserve_position(world, to);
        for (EcsSizeT it = 0; it < ECS_WORLD_MAX_COMPONENTS; it++)
        {
            if (!fromArchetype->componentFlags.get_flag(it))
            {
                continue;
            }
            uint8_t* fromComponent = ecs_access_component(world, from, it, fromIndex);
            uint8_t* toComponent = ecs_access_component(world, to, it, toIndex);
            std::memcpy(toComponent, fromComponent, gEcsComponentInfos[it].sizeBytes);
        }
        *get(&toArchetype->entityHandles, toIndex) = handle;
        ecs_free_position(world, from, fromIndex);
        entityPtr->archetypeHandle = to;
        entityPtr->archetypeArrayIndex = toIndex;
    }

    void ecs_move_entity_subset(EcsWorld* world, EcsArchetypeHandle from, EcsArchetypeHandle to, EcsEntityHandle handle)
    {
        EcsArchetype*   fromArchetype   = get(&world->archetypes, from);
        EcsArchetype*   toArchetype     = get(&world->archetypes, to);
        EcsEntity*      entityPtr       = get(&world->entities, handle);
        EcsSizeT        fromIndex       = entityPtr->archetypeArrayIndex;
        EcsSizeT        toIndex         = ecs_reserve_position(world, to);
        for (EcsSizeT it = 0; it < ECS_WORLD_MAX_COMPONENTS; it++)
        {
            if (!toArchetype->componentFlags.get_flag(it))
            {
                continue;
            }
            uint8_t* fromComponent = ecs_access_component(world, from, it, fromIndex);
            uint8_t* toComponent = ecs_access_component(world, to, it, toIndex);
            std::memcpy(toComponent, fromComponent, gEcsComponentInfos[it].sizeBytes);
        }
        *get(&toArchetype->entityHandles, toIndex) = handle;
        ecs_free_position(world, from, fromIndex);
        entityPtr->archetypeHandle = to;
        entityPtr->archetypeArrayIndex = toIndex;
    }

    EcsSizeT ecs_reserve_position(EcsWorld* world, EcsArchetypeHandle handle)
    {
        if (handle == ECS_WORLD_EMPTY_ARCHETYPE)
        {
            return 0;
        }
        EcsArchetype* archetype = get(&world->archetypes, handle);
        EcsSizeT position = archetype->size;
        archetype->size += 1;
        if (archetype->size >= archetype->capacity)
        {
            ecs_allocate_chunks(world, handle);
        }
        return position;
    }

    void ecs_free_position(EcsWorld* world, EcsArchetypeHandle handle, EcsSizeT index)
    {
        if (handle == ECS_WORLD_EMPTY_ARCHETYPE)
        {
            return;
        }
        EcsArchetype* archetype = get(&world->archetypes, handle);
        al_assert(archetype->size != 0);
        if (index == (archetype->size - 1))
        {
            for (EcsSizeT it = 0; it < ECS_WORLD_MAX_COMPONENTS; it++)
            {
                if (!archetype->componentFlags.get_flag(it))
                {
                    continue;
                }
                uint8_t* component = ecs_access_component(world, handle, it, index);
                std::memset(component, 0, gEcsComponentInfos[it].sizeBytes);
            }
            archetype->size -= 1;
            return;
        }
        EcsSizeT lastIndex = archetype->size - 1;
        for (EcsSizeT it = 0; it < ECS_WORLD_MAX_COMPONENTS; it++)
        {
            if (!archetype->componentFlags.get_flag(it))
            {
                continue;
            }
            uint8_t* fromComponent = ecs_access_component(world, handle, it, lastIndex);
            uint8_t* toComponent = ecs_access_component(world, handle, it, index);
            EcsSizeT sizeBytes = gEcsComponentInfos[it].sizeBytes;
            std::memcpy(toComponent, fromComponent, sizeBytes);
            std::memset(fromComponent, 0, sizeBytes);
        }
        EcsEntityHandle* lastHandle = get(&archetype->entityHandles, lastIndex);
        *get(&archetype->entityHandles, index) = *lastHandle;
        al_memzero(lastHandle);
        archetype->size -= 1;
    }

    uint8_t* ecs_access_component(EcsWorld* world, EcsArchetypeHandle handle, EcsComponentId componentId, EcsSizeT index)
    {
        EcsArchetype* archetype = get(&world->archetypes, handle);
        if (!archetype->componentFlags.get_flag(componentId))
        {
            return nullptr;
        }
        if (archetype->size <= index)
        {
            return nullptr;
        }
        EcsSizeT chunkIndex = index / archetype->singleChunkCapacity;
        EcsSizeT inChunkIndex = index % archetype->singleChunkCapacity;
        return *get(&archetype->chunks, chunkIndex) + *get(&archetype->componentArrayPointers, componentId) + inChunkIndex * gEcsComponentInfos[componentId].sizeBytes;
    }

    template<typename T>
    T* ecs_access_component(EcsArchetype* archetype, EcsSizeT index)
    {
        EcsSizeT chunkIndex = index / archetype->singleChunkCapacity;
        EcsSizeT inChunkIndex = index % archetype->singleChunkCapacity;
        EcsComponentId componentId = ecs_component_type_info_get_id<T>();
        uint8_t* memory = *get(&archetype->chunks, chunkIndex) + *get(&archetype->componentArrayPointers, componentId) + inChunkIndex * gEcsComponentInfos[componentId].sizeBytes;
        return reinterpret_cast<T*>(memory);
    };

    template<typename T>
    inline EcsComponentId ecs_component_type_info_get_id()
    {
        static EcsComponentId id = gEcsComponentCount++;
        return id;
    }

    template<typename T>
    inline EcsSizeT ecs_component_type_info_get_size()
    {
        return sizeof(T);
    }

    // @NOTE :  Component size of zero is considered invalid.
    template<typename T>
    inline bool ecs_is_component_registered()
    {
        return gEcsComponentInfos[ecs_component_type_info_get_id<T>()].sizeBytes != 0;
    }

    template<typename T, typename ... U> bool ecs_is_components_registered(bool value)
    {
        bool result = value && ecs_is_component_registered<T>();
        if constexpr (sizeof...(U) != 0)
        {
            result = result && is_components_registered<U...>();
        }
        return result;
    }

    template<typename T>
    inline void ecs_register_component()
    {
        al_assert(ecs_component_type_info_get_id<T>() < ECS_WORLD_MAX_COMPONENTS);
        gEcsComponentInfos[ecs_component_type_info_get_id<T>()].sizeBytes = ecs_component_type_info_get_size<T>();
    }

    template<typename T, typename ... U>
    void ecs_register_components_if_needed()
    {
        if (!ecs_is_component_registered<T>())
        {
            ecs_register_component<T>();
        }
        if constexpr (sizeof...(U) != 0)
        {
            ecs_register_components_if_needed<U...>();
        }
    }

    template<typename T, typename ... U>
    void ecs_set_component_flags(EcsComponentFlags* flags)
    {
        flags->set_flag(ecs_component_type_info_get_id<T>());
        if constexpr (sizeof...(U) != 0)
        {
            ecs_set_component_flags<U...>(flags);
        }
    }

    template<typename T, typename ... U>
    void ecs_clear_component_flags(EcsComponentFlags* flags)
    {
        flags->clear_flag(ecs_component_type_info_get_id<T>());
        if constexpr (sizeof...(U) != 0)
        {
            ecs_clear_component_flags<U...>(flags);
        }
    }

    bool ecs_is_valid_subset(EcsComponentFlags subset, EcsComponentFlags superset)
    {
        return  ((subset.flags[0] & superset.flags[0]) == subset.flags[0]) &&
                ((subset.flags[1] & superset.flags[1]) == subset.flags[1]);
    }
}
