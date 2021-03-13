#ifndef AL_ECS_H
#define AL_ECS_H

#include <cstdint>

#include "engine/config/engine_config.h"
#include "engine/memory/memory_common.h"
#include "engine/debug/debug.h"
#include "engine/containers/containers.h"

#include "utilities/flags.h"
#include "utilities/function.h"

namespace al::engine
{
    using EcsSizeT              = uint64_t;
    using EcsEntityHandle       = EcsSizeT;
    using EcsArchetypeHandle    = EcsSizeT;
    using EcsComponentId        = EcsSizeT;
    using EcsComponentFlags     = Flags128;

    template<typename ... T> using EcsForEachFunctionPointer    = void(*)(struct EcsWorld*, EcsEntityHandle, T*...);
    template<typename ... T> using EcsForEachFunctionObject     = Function<void(struct EcsWorld*, EcsEntityHandle, T*...)>;

    // @NOTE :  Must correlate with EcsComponentFlags type.
    //          One is subtracted because ComponentCounter::count
    //          value starts from one, not zero.
    constexpr EcsSizeT              ECS_WORLD_MAX_COMPONENTS    = 128 - 1;
    constexpr EcsArchetypeHandle    ECS_WORLD_EMPTY_ARCHETYPE   = 0 ;

    extern        EcsComponentId            gEcsComponentCount;
    extern struct EcsComponentRuntimeInfo   gEcsComponentInfos[ECS_WORLD_MAX_COMPONENTS];

    struct al_align EcsComponentRuntimeInfo
    {
        EcsSizeT sizeBytes = 0;
    };

    // @NOTE :  EcsEntity has the following data.
    //          componentFlags - describes the components that this entity has.
    //          archetypeHandle - handle to an archetype which stores this entity.
    //          archetypeArrayIndex - index af entity components in archetype component arrays.
    struct al_align EcsEntity
    {
        EcsComponentFlags   componentFlags;
        EcsArchetypeHandle  archetypeHandle;
        EcsSizeT            archetypeArrayIndex;
    };

    struct al_align EcsArchetype
    {
        EcsComponentFlags                                                               componentFlags;         // 16
        EcsArchetypeHandle                                                              selfHandle;             // 8
        EcsSizeT                                                                        size;                   // 8
        EcsSizeT                                                                        capacity;               // 8
        EcsSizeT                                                                        singleChunkCapacity;    // 8
        ArrayView<EcsSizeT, ECS_WORLD_MAX_COMPONENTS>                                   componentArrayPointers; // 8
        ArrayView<EcsEntityHandle, EngineConfig::ECS_MAX_ENTITIES_IN_ARCHETYPE_CHUNK>   entityHandles;          // 8
        DynamicArray<uint8_t*>                                                          chunks;                 // 32
    };

    struct al_align EcsWorld
    {
        // Theese pools are used in archetypes
        EcsSizeT        componentArrayPointersPool  [ECS_WORLD_MAX_COMPONENTS * EngineConfig::ECS_MAX_ARCHETYPES];
        EcsEntityHandle entityHandlesPool           [EngineConfig::ECS_MAX_ENTITIES_IN_ARCHETYPE_CHUNK * EngineConfig::ECS_MAX_ARCHETYPES];

        ArrayContainer<EcsEntity, EngineConfig::ECS_MAX_ENTITIES>       entities;
        ArrayContainer<EcsArchetype, EngineConfig::ECS_MAX_ARCHETYPES>  archetypes;
    };

    // =================================================================================================================================
    // USER STUFF
    // =================================================================================================================================

    void construct(EcsWorld* world);
    void destruct(EcsWorld* world);

                                EcsEntityHandle ecs_create_entity       (EcsWorld* world);
    template<typename ... T>    void            ecs_add_components      (EcsWorld* world, EcsEntityHandle handle);
    template<typename ... T>    void            ecs_remove_components   (EcsWorld* world, EcsEntityHandle handle);
    template<typename T>        T*              ecs_get_component       (EcsWorld* world, EcsEntityHandle handle);

    // // @NOTE :  This version of for_each is faster than the one below
    template<typename ... T>    void            ecs_for_each_fp         (EcsWorld* world, EcsForEachFunctionPointer<T...> func);
    template<typename ... T>    void            ecs_for_each            (EcsWorld* world, EcsForEachFunctionObject<T...> func);

    // =================================================================================================================================
    // INNER STUFF
    // =================================================================================================================================

    EcsArchetypeHandle  ecs_match_or_create_archetype   (EcsWorld* world, EcsEntityHandle handle);
    EcsArchetypeHandle  ecs_create_archetype            (EcsWorld* world, EcsComponentFlags flags);
    void                ecs_allocate_chunks             (EcsWorld* world, EcsArchetypeHandle handle);
    void                ecs_move_entity_superset        (EcsWorld* world, EcsArchetypeHandle from, EcsArchetypeHandle to, EcsEntityHandle handle);
    void                ecs_move_entity_subset          (EcsWorld* world, EcsArchetypeHandle from, EcsArchetypeHandle to, EcsEntityHandle handle);
    EcsSizeT            ecs_reserve_position            (EcsWorld* world, EcsArchetypeHandle handle);
    void                ecs_free_position               (EcsWorld* world, EcsArchetypeHandle handle, EcsSizeT index);
    uint8_t*            ecs_access_component           (EcsWorld* world, EcsArchetypeHandle handle, EcsComponentId componentId, EcsSizeT index);

    template<typename T>
    T* ecs_access_component(EcsArchetype* archetype, EcsSizeT index);

    template<typename T>                    EcsComponentId  ecs_component_type_info_get_id      ();
    template<typename T>                    EcsSizeT        ecs_component_type_info_get_size    ();
    template<typename T>                    bool            ecs_is_component_registered         ();
    template<typename T, typename ... U>    bool            ecs_is_components_registered        (bool value = true);
    template<typename T>                    void            ecs_register_component              ();
    template<typename T, typename ... U>    void            ecs_register_components_if_needed   ();
    template<typename T, typename ... U>    void            ecs_set_component_flags             (EcsComponentFlags* flags);
    template<typename T, typename ... U>    void            ecs_clear_component_flags           (EcsComponentFlags* flags);
                                            bool            ecs_is_valid_subset                 (EcsComponentFlags subset, EcsComponentFlags superset);
}

#endif
