#ifndef AL_ECS_H
#define AL_ECS_H

#include <cstdint>

#include "engine/config/engine_config.h"
#include "engine/memory/memory_common.h"
#include "engine/debug/debug.h"
#include "engine/containers/containers.h"

#include "utilities/non_copyable.h"
#include "utilities/array_container.h"
#include "utilities/flags.h"
#include "utilities/function.h"

namespace al::engine
{
    using EcsSizeT      = uint64_t;
    using EntityHandle  = EcsSizeT;

    class EcsWorld : public NonCopyable
    {
    private:
        using ArchetypeHandle   = EcsSizeT;
        using ComponentId       = EcsSizeT;
        using ComponentFlags    = Flags128;

        // @NOTE :  Must correlate with ComponentFlags type.
        //          One is subtracted because ComponentCounter::count
        //          value starts from one, not zero.
        static constexpr EcsSizeT MAX_COMPONENTS{ 128 - 1 };

        // @NOTE :  Entity has the following data.
        //          componentFlags - describes the components that this entity has.
        //          archetypeHandle - handle to an archetype which stores this entity.
        //          arrayIndex - index af entity components in archetype component arrays.
        struct al_align Entity
        {
            ComponentFlags componentFlags;
            ArchetypeHandle archetypeHandle;
            EcsSizeT arrayIndex;
        };

        struct ComponentCounter { static ComponentId count; };

        template<typename T>
        class ComponentTypeInfo : public ComponentCounter
        {
        public:
            inline static ComponentId get_id() noexcept
            {
                static ComponentId id = count++;
                return id;
            }

            inline static EcsSizeT get_size() noexcept
            {
                return sizeof(T);
            }
        };

        struct al_align ComponentRuntimeInfo
        {
            EcsSizeT sizeBytes = 0;
        };

        // @NOTE :  ComponentArray stores instances of a single component type.
        //          Instances are stored in chunks. Each chunk has size of
        //          EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK * sizeof(*COMPONENT TYPE*).
        //          Pointers to chunks are stored as uint8_t* and reinterpreted as pointers of 
        //          *COMPONENT TYPE* at runtime via templated methods (accsess_component_template).
        struct ComponentArray
        {
            ComponentId componentId;
            DynamicArray<uint8_t*> chunks;
        };

        // @NOTE :  Archetype has the following data.
        //          componentFlags - describes the components of entities that stored in this archetype.
        //          selfHandle - handle of this archetype in the archetypes array.
        //          size - current number of entities in arhcetype.
        //          capacity - max possible number of stored entities without additional allocations.
        //          entityHandlesChunks - chunks of entity handles. There stored handles to entities that
        //                                are belong to this archetype.
        //          components - array of the component arrays.
        struct al_align Archetype
        {
            ComponentFlags componentFlags;
            ArchetypeHandle selfHandle;
            EcsSizeT size;
            EcsSizeT capacity;
            DynamicArray<EntityHandle*> entityHandlesChunks;
            ComponentArray components[MAX_COMPONENTS];
        };

    public:
        EcsWorld() noexcept;
        ~EcsWorld() noexcept;

                                    void            log_world_state     ()                      noexcept;
                                    EntityHandle    create_entity       ()                      noexcept;
                                    // @TODO :  maybe return tuple with added components from this method ?
        template<typename ... T>    void            add_components      (EntityHandle handle)   noexcept;
        template<typename ... T>    void            remove_components   (EntityHandle handle)   noexcept;
        template<typename T>        T*              get_component       (EntityHandle handle)   noexcept;

        template<typename ... T>
        using ForEachFunction = void(*)(EcsWorld*, EntityHandle, T*...);

        // @NOTE :  This version of for_each is faster than the one below
        template<typename ... T>
        void for_each_fp(ForEachFunction<T...> func) noexcept;

        template<typename ... T>
        void for_each(Function<void(EcsWorld*, EntityHandle, T*...)> func) noexcept;

    private:
        static constexpr ArchetypeHandle EMPTY_ARCHETYPE{ 0 };

        ComponentRuntimeInfo    componentInfos[MAX_COMPONENTS];
        DynamicArray<Entity>    entities;
        DynamicArray<Archetype> archetypes;

                                                Entity*                 entity                          (EntityHandle handle)                                           noexcept;
                                                Archetype*              archetype                       (ArchetypeHandle handle)                                        noexcept;
                                                ComponentRuntimeInfo*   component_info                  (ComponentId componentId)                                       noexcept;
        template<typename T>                    bool                    is_component_registered         ()                                                              noexcept;
        template<typename T, typename ... U>    bool                    is_components_registered        (bool value = true)                                             noexcept;
        template<typename T>                    void                    register_component              ()                                                              noexcept;
        template<typename T, typename ... U>    void                    register_components_if_needed   ()                                                              noexcept;
        template<typename T, typename ... U>    void                    set_component_flags             (ComponentFlags* flags)                                         noexcept;
        template<typename T, typename ... U>    void                    clear_component_flags           (ComponentFlags* flags)                                         noexcept;
                                                ArchetypeHandle         match_or_create_archetype       (EntityHandle handle)                                           noexcept;
                                                ArchetypeHandle         create_archetype                (ComponentFlags flags)                                          noexcept;
                                                void                    allocate_chunks                 (ArchetypeHandle handle)                                        noexcept;
                                                EcsSizeT                reserve_position                (ArchetypeHandle handle)                                        noexcept;
                                                void                    free_position                   (ArchetypeHandle handle, EcsSizeT index)                        noexcept;
                                                void                    move_entity_superset            (ArchetypeHandle from, ArchetypeHandle to, EntityHandle handle) noexcept;
                                                void                    move_entity_subset              (ArchetypeHandle from, ArchetypeHandle to, EntityHandle handle) noexcept;
        template<typename T>                    T*                      accsess_component_template      (Archetype* archetypePtr, EcsSizeT index)                       noexcept;
        template<typename T>                    T*                      accsess_component_template      (ComponentArray* array, EcsSizeT index)                         noexcept;
                                                uint8_t*                accsess_component               (ComponentArray* array, EcsSizeT index)                         noexcept;
                                                EntityHandle*           accsess_entity_handle           (ArchetypeHandle handle, EcsSizeT index)                        noexcept;
                                                EntityHandle*           accsess_entity_handle           (Archetype* archetypePtr, EcsSizeT index)                       noexcept;
                                                ComponentArray*         accsess_component_array         (ArchetypeHandle handle, ComponentId componentId)               noexcept;
                                                bool                    is_valid_subset                 (ComponentFlags subset, ComponentFlags superset)                noexcept;
                                                bool                    is_valid_component_array        (ComponentArray* array)                                         noexcept;
    };
}

#endif
