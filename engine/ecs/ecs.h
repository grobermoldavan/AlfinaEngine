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
    using ComponentFlags = Flags128;
    constexpr uint64_t MAX_COMPONENTS{ 128 };

    using EntityHandle = uint64_t;
    using ArchetypeHandle = uint64_t;

    template <typename T> concept entity_handle = std::is_same_v<EntityHandle, T>;

    struct al_align Entity
    {
        ComponentFlags componentFlags;
        ArchetypeHandle archetypeHandle;
        uint64_t arrayIndex;
    };

    class ComponentCounter
    {
    protected:
        static uint64_t count;
    };

    template<typename T>
    class ComponentTypeInfo : public ComponentCounter
    {
    public:
        inline static uint64_t get_id() noexcept
        {
            static uint64_t id = count++;
            return id;
        }

        inline static uint64_t get_size() noexcept
        {
            return sizeof(T);
        }
    };

    struct al_align ComponentRuntimeInfo
    {
        uint64_t sizeBytes = 0;
    };

    struct ComponentArray
    {
        uint64_t componentId;
        DynamicArray<uint8_t*> chunks;
    };

    struct al_align Archetype
    {
        ComponentFlags componentFlags;
        ArchetypeHandle selfHandle;
        uint64_t size;
        uint64_t capacity;
        DynamicArray<EntityHandle*> entityHandlesChunks;
        ComponentArray components[MAX_COMPONENTS];
    };

    class EcsWorld : public NonCopyable
    {
    public:
        EcsWorld() noexcept;
        ~EcsWorld() noexcept;

                                    void            log_world_state     ()                      noexcept;
                                    EntityHandle    create_entity       ()                      noexcept;
        template<typename ... T>    void            add_components      (EntityHandle handle)   noexcept;
        template<typename ... T>    void            remove_components   (EntityHandle handle)   noexcept;
        template<typename T>        T*              get_component       (EntityHandle handle)   noexcept;

        template<typename ... T>
        using ForEachFunction = void(*)(EcsWorld*, EntityHandle, T*...);

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
                                                ComponentRuntimeInfo*   component_info                  (uint64_t componentId)                                          noexcept;
        template<typename T>                    bool                    is_component_registered         ()                                                              noexcept;
        template<typename T, typename ... U>    bool                    is_components_registered        (bool value = true)                                             noexcept;
        template<typename T>                    void                    register_component              ()                                                              noexcept;
        template<typename T, typename ... U>    void                    register_components_if_needed   ()                                                              noexcept;
        template<typename T, typename ... U>    void                    set_component_flags             (ComponentFlags* flags)                                         noexcept;
        template<typename T, typename ... U>    void                    clear_component_flags           (ComponentFlags* flags)                                         noexcept;
                                                ArchetypeHandle         match_or_create_archetype       (EntityHandle handle)                                           noexcept;
                                                ArchetypeHandle         create_archetype                (ComponentFlags flags)                                          noexcept;
                                                void                    allocate_chunks                 (ArchetypeHandle handle)                                        noexcept;
                                                uint64_t                reserve_position                (ArchetypeHandle handle)                                        noexcept;
                                                void                    free_position                   (ArchetypeHandle handle, uint64_t index)                        noexcept;
                                                void                    move_entity_superset            (ArchetypeHandle from, ArchetypeHandle to, EntityHandle handle) noexcept;
                                                void                    move_entity_subset              (ArchetypeHandle from, ArchetypeHandle to, EntityHandle handle) noexcept;
        template<typename T>                    T*                      accsess_component_template      (Archetype* archetypePtr, uint64_t index)                       noexcept;
        template<typename T>                    T*                      accsess_component_template      (ComponentArray* array, uint64_t index)                         noexcept;
                                                uint8_t*                accsess_component               (ComponentArray* array, uint64_t index)                         noexcept;
                                                EntityHandle*           accsess_entity_handle           (ArchetypeHandle handle, uint64_t index)                        noexcept;
                                                EntityHandle*           accsess_entity_handle           (Archetype* archetypePtr, uint64_t index)                       noexcept;
                                                ComponentArray*         accsess_component_array         (ArchetypeHandle handle, uint64_t componentId)                  noexcept;
                                                bool                    is_valid_subset                 (ComponentFlags subset, ComponentFlags superset)                noexcept;
                                                bool                    is_valid_component_array        (ComponentArray* array)                                         noexcept;
    };
}

#endif
