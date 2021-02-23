
#include <inttypes.h>

#include "ecs.h"

#include "engine/memory/memory_manager.h"
#include "engine/debug/debug.h"

#include "utilities/constexpr_functions.h"

namespace al::engine
{
    // @NOTE :  Component count starts with number one, not zero.
    //          This is done because ECS system thinks that component array
    //          with componentId == 0 is not used (see is_valid_component_array).
    //          We could use additional bool value (isValid or something), but
    //          currently this is not very neccessary.
    EcsWorld::ComponentId EcsWorld::ComponentCounter::count{ 1 };

    EcsWorld::EcsWorld() noexcept
        : componentInfos{ }
        , entities{ }
        , archetypes{ Archetype{ ComponentFlags{ }, EMPTY_ARCHETYPE } }
    { }

    EcsWorld::~EcsWorld() noexcept
    { }

    void EcsWorld::log_world_state() noexcept
    {
        al_log_message(EngineConfig::ECS_LOG_CATEGORY, "============================================================");
        al_log_message(EngineConfig::ECS_LOG_CATEGORY, "Current ECS world state is :");
        for (EcsSizeT it = 1; it < archetypes.size(); it++)
        {
            al_log_message(EngineConfig::ECS_LOG_CATEGORY, "Archetype %d :", it);
            al_log_message(EngineConfig::ECS_LOG_CATEGORY, "    Size        : %d", archetype(it)->size);
            al_log_message(EngineConfig::ECS_LOG_CATEGORY, "    Capacity    : %d", archetype(it)->capacity);
            al_log_message(EngineConfig::ECS_LOG_CATEGORY, "    Flags       : %d %d", archetype(it)->componentFlags.flags[0], archetype(it)->componentFlags.flags[1]);
            al_log_message(EngineConfig::ECS_LOG_CATEGORY, "    Components  : ");
            for (ComponentArray& array : archetype(it)->components)
            {
                if (!is_valid_component_array(&array))
                {
                    continue;
                }
                al_log_message(EngineConfig::ECS_LOG_CATEGORY, "        Component with id : %d", array.componentId);
                EcsSizeT memoryAllocated = component_info(array.componentId)->sizeBytes * array.chunks.size() * EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK;
                al_log_message(EngineConfig::ECS_LOG_CATEGORY, "        Memory allocated  : %d bytes", memoryAllocated);
            }
        }
        al_log_message(EngineConfig::ECS_LOG_CATEGORY, "============================================================");
    }

    EntityHandle EcsWorld::create_entity() noexcept
    {
        EntityHandle handle = entities.size();
        entities.push_back
        ({
            .componentFlags = { },
            .archetypeHandle = 0,
            .arrayIndex = 0
        });
        return handle;
    }

    // @NOTE :  Process of adding components consists of several steps.
    //          First, we save current entity component flags and make new
    //          ones by adding flags of the components passed to a method.
    //          If flags equal, we simply return because all needed components
    //          are already added to an entity.
    //          If flags not equal, we retrieve current entity archetype, then
    //          we retrive (get or create) new archetype and move entity from
    //          one archetype to another.
    template<typename ... T>
    void EcsWorld::add_components(EntityHandle handle) noexcept
    {
        register_components_if_needed<T...>();
        ComponentFlags currentFlags = entity(handle)->componentFlags;
        ComponentFlags newFlags = entity(handle)->componentFlags;
        set_component_flags<T...>(&newFlags);
        if (currentFlags == newFlags)
        {
            return;
        }
        entity(handle)->componentFlags = newFlags;
        ArchetypeHandle oldArchetype = entity(handle)->archetypeHandle;
        ArchetypeHandle newArchetype = match_or_create_archetype(handle);
        move_entity_superset(oldArchetype, newArchetype, handle);
    }

    // @NOTE :  Process of removing components is pretty similar to
    //          the process of adding components. But instead of adding
    //          flags we remove them.
    template<typename ... T>
    void EcsWorld::remove_components(EntityHandle handle) noexcept
    {
        al_assert(is_components_registered<T...>());
        ComponentFlags currentFlags = entity(handle)->componentFlags;
        ComponentFlags newFlags = entity(handle)->componentFlags;
        clear_component_flags<T...>(&newFlags);
        if (currentFlags == newFlags)
        {
            return;
        }
        entity(handle)->componentFlags = newFlags;
        ArchetypeHandle oldArchetype = entity(handle)->archetypeHandle;
        ArchetypeHandle newArchetype = match_or_create_archetype(handle);
        move_entity_subset(oldArchetype, newArchetype, handle);
    }

    // @NOTE :  Process of getting component is pretty straightforward.
    //          First we check if component registerered in our system
    //          and if entity actually has that component. After that
    //          we accsess component array of entity archetype and
    //          get the value from that array by index.
    template<typename T>
    T* EcsWorld::get_component(EntityHandle handle) noexcept
    {
        al_assert( is_component_registered<T>() );
        al_assert( entity(handle)->componentFlags.get_flag(ComponentTypeInfo<T>::get_id()) );
        ComponentArray* array = accsess_component_array(entity(handle)->archetypeHandle, ComponentTypeInfo<T>::get_id());
        return accsess_component_template<T>(array, entity(handle)->arrayIndex);
    }

    // @NOTE :  Iterating over some set of components is done the following way.
    //          First we create component flags of the requested set of components.
    //          Then we iterate over each archetype and check if this archetype
    //          contains all required components. If it does, we iterate over each
    //          entity in that archetype and call user function.
    template<typename ... T>
    void EcsWorld::for_each_fp(ForEachFunction<T...> func) noexcept
    {
        ComponentFlags requestFlags{ };
        set_component_flags<T...>(&requestFlags);
        const EcsSizeT archetypesSize = archetypes.size();
        for (EcsSizeT it = 1; it < archetypesSize; it++)
        {
            if (!is_valid_subset(requestFlags, archetypes[it].componentFlags))
            {
                continue;
            }
            Archetype* archetypePtr = &archetypes[it];
            const EcsSizeT archetypeSize = archetypePtr->size;
            for (EcsSizeT entityIt = 0; entityIt < archetypeSize; entityIt++)
            {
                func(this, *accsess_entity_handle(archetypePtr, entityIt), accsess_component_template<T>(archetypePtr, entityIt)...);
            }
        }
    }

    // @NOTE :  same as for_each_fp but with Function object provided by user
    //          instead of function pointer.
    template<typename ... T>
    void EcsWorld::for_each(Function<void(EcsWorld*, EntityHandle, T*...)> func) noexcept
    {
        ComponentFlags requestFlags{ };
        set_component_flags<T...>(&requestFlags);
        const EcsSizeT archetypesSize = archetypes.size();
        for (EcsSizeT it = 1; it < archetypesSize; it++)
        {
            if (!is_valid_subset(requestFlags, archetypes[it].componentFlags))
            {
                continue;
            }
            Archetype* archetypePtr = &archetypes[it];
            const EcsSizeT archetypeSize = archetypePtr->size;
            for (EcsSizeT entityIt = 0; entityIt < archetypeSize; entityIt++)
            {
                func(this, *accsess_entity_handle(archetypePtr, entityIt), accsess_component_template<T>(archetypePtr, entityIt)...);
            }
        }
    }

    inline EcsWorld::Entity* EcsWorld::entity(EntityHandle handle) noexcept
    {
        return &entities[handle];
    }

    inline EcsWorld::Archetype* EcsWorld::archetype(ArchetypeHandle handle) noexcept
    {
        return &archetypes[handle];
    }

    inline EcsWorld::ComponentRuntimeInfo* EcsWorld::component_info(ComponentId componentId) noexcept
    {
        return &componentInfos[componentId];
    }

    // @NOTE :  Component size of zero is considered invalid.
    template<typename T>
    inline bool EcsWorld::is_component_registered() noexcept
    {
        return component_info(ComponentTypeInfo<T>::get_id())->sizeBytes != 0;
    }

    template<typename T, typename ... U> bool EcsWorld::is_components_registered(bool value) noexcept
    {
        bool result = value && is_component_registered<T>();
        if constexpr (sizeof...(U) != 0)
        {
            result = result && is_components_registered<U...>();
        }
        return result;
    }

    template<typename T>
    inline void EcsWorld::register_component() noexcept
    {
        al_assert(ComponentTypeInfo<T>::get_id() < MAX_COMPONENTS);
        component_info(ComponentTypeInfo<T>::get_id())->sizeBytes = ComponentTypeInfo<T>::get_size();
    }

    template<typename T, typename ... U>
    void EcsWorld::register_components_if_needed() noexcept
    {
        if (!is_component_registered<T>())
        {
            register_component<T>();
        }
        if constexpr (sizeof...(U) != 0)
        {
            register_components_if_needed<U...>();
        }
    }

    template<typename T, typename ... U>
    void EcsWorld::set_component_flags(ComponentFlags* flags) noexcept
    {
        flags->set_flag(ComponentTypeInfo<T>::get_id());
        if constexpr (sizeof...(U) != 0)
        {
            set_component_flags<U...>(flags);
        }
    }

    template<typename T, typename ... U>
    void EcsWorld::clear_component_flags(ComponentFlags* flags) noexcept
    {
        flags->clear_flag(ComponentTypeInfo<T>::get_id());
        if constexpr (sizeof...(U) != 0)
        {
            clear_component_flags<U...>(flags);
        }
    }

    EcsWorld::ArchetypeHandle EcsWorld::match_or_create_archetype(EntityHandle handle) noexcept
    {
        for (Archetype& archetype : archetypes)
        {
            if (archetype.componentFlags == entity(handle)->componentFlags)
            {
                return archetype.selfHandle;
            }
        }
        return create_archetype(entity(handle)->componentFlags);
    }

    EcsWorld::ArchetypeHandle EcsWorld::create_archetype(ComponentFlags flags) noexcept
    {
        ArchetypeHandle handle = archetypes.size();
        archetypes.push_back
        ({
            .componentFlags = flags,
            .selfHandle = handle,
            .size = 0,
            .capacity = 0,
            .entityHandlesChunks = { },
            .components = { 0 }
        });
        Archetype* archetypePtr = archetype(handle);
        for (EcsSizeT it = 0; it < MAX_COMPONENTS; it++)
        {
            if (!archetypePtr->componentFlags.get_flag(it))
            {
                continue;
            }
            archetypePtr->components[it] =
            {
                .componentId = it,
                .chunks = { }
            };
        }
        allocate_chunks(handle);
        return handle;
    }

    void EcsWorld::allocate_chunks(ArchetypeHandle handle) noexcept
    {
        Archetype* archetypePtr = archetype(handle);
        for (ComponentArray& array : archetypePtr->components)
        {
            if (!is_valid_component_array(&array))
            {
                continue;
            }
            const EcsSizeT componentSize = component_info(array.componentId)->sizeBytes;
            std::byte* memory = MemoryManager::get_pool()->allocate(componentSize * EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK);
            std::memset(memory, 0, componentSize * EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK);
            array.chunks.push_back(reinterpret_cast<uint8_t*>(memory));
        }
        std::byte* memory = MemoryManager::get_pool()->allocate(sizeof(EntityHandle) * EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK);
        std::memset(memory, 0, sizeof(EntityHandle) * EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK);
        archetypePtr->entityHandlesChunks.push_back(reinterpret_cast<EntityHandle*>(memory));
        archetypePtr->capacity += EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK;
    }

    EcsSizeT EcsWorld::reserve_position(ArchetypeHandle handle) noexcept
    {
        Archetype* archetypePtr = archetype(handle);
        EcsSizeT position = archetypePtr->size;
        archetypePtr->size += 1;
        if (archetypePtr->size >= archetypePtr->capacity)
        {
            allocate_chunks(handle);
        }
        return position;
    }

    void EcsWorld::free_position(ArchetypeHandle handle, EcsSizeT index) noexcept
    {
        if (handle == EMPTY_ARCHETYPE)
        {
            return;
        }
        Archetype* archetypePtr = archetype(handle);
        al_assert(archetypePtr->size != 0);
        if (index == (archetypePtr->size - 1))
        {
            for (ComponentArray& array : archetypePtr->components)
            {
                if (!is_valid_component_array(&array))
                {
                    continue;
                }
                uint8_t* component = accsess_component(&array, index);
                std::memset(component, 0, component_info(array.componentId)->sizeBytes);
            }
            archetypePtr->size -= 1;
            return;
        }
        EcsSizeT lastIndex = archetypePtr->size - 1;
        for (ComponentArray& array : archetypePtr->components)
        {
            if (!is_valid_component_array(&array))
            {
                continue;
            }
            uint8_t* fromComponent = accsess_component(&array, lastIndex);
            uint8_t* toComponent = accsess_component(&array, index);
            std::memcpy(toComponent, fromComponent, component_info(array.componentId)->sizeBytes);
            std::memset(fromComponent, 0, component_info(array.componentId)->sizeBytes);
        }
        *accsess_entity_handle(archetypePtr, index) = *accsess_entity_handle(archetypePtr, lastIndex);
        std::memset(accsess_entity_handle(handle, lastIndex), 0, sizeof(EntityHandle));
        archetypePtr->size -= 1;
    }

    void EcsWorld::move_entity_superset(ArchetypeHandle from, ArchetypeHandle to, EntityHandle handle) noexcept
    {
        Archetype* fromPtr = archetype(from);
        Archetype* toPtr = archetype(to);
        Entity* entityPtr = entity(handle);
        EcsSizeT fromIndex = entityPtr->arrayIndex;
        EcsSizeT toIndex = reserve_position(to);
        for (ComponentArray& fromArray : fromPtr->components)
        {
            if (!is_valid_component_array(&fromArray))
            {
                continue;
            }
            ComponentArray* toArray = accsess_component_array(to, fromArray.componentId);
            uint8_t* fromComponent = accsess_component(&fromArray, fromIndex);
            uint8_t* toComponent = accsess_component(toArray, toIndex);
            std::memcpy(toComponent, fromComponent, component_info(fromArray.componentId)->sizeBytes);
        }
        *accsess_entity_handle(toPtr, toIndex) = handle;
        free_position(from, fromIndex);
        entityPtr->archetypeHandle = to;
        entityPtr->arrayIndex = toIndex;
    }

    void EcsWorld::move_entity_subset(ArchetypeHandle from, ArchetypeHandle to, EntityHandle handle) noexcept
    {
        Archetype* fromPtr = archetype(from);
        Archetype* toPtr = archetype(to);
        Entity* entityPtr = entity(handle);
        EcsSizeT fromIndex = entityPtr->arrayIndex;
        EcsSizeT toIndex = reserve_position(to);
        for (ComponentArray& toArray : toPtr->components)
        {
            if (!is_valid_component_array(&toArray))
            {
                continue;
            }
            ComponentArray* fromArray = accsess_component_array(from, toArray.componentId);
            uint8_t* fromComponent = accsess_component(fromArray, fromIndex);
            uint8_t* toComponent = accsess_component(&toArray, toIndex);
            std::memcpy(toComponent, fromComponent, component_info(toArray.componentId)->sizeBytes);
        }
        *accsess_entity_handle(toPtr, toIndex) = handle;
        free_position(from, fromIndex);
        entityPtr->archetypeHandle = to;
        entityPtr->arrayIndex = toIndex;
    }

    template<typename T>
    inline T* EcsWorld::accsess_component_template(Archetype* archetypePtr, EcsSizeT index) noexcept
    {
        return accsess_component_template<T>(&archetypePtr->components[ComponentTypeInfo<T>::get_id()], index);
    }

    template<typename T>
    inline T* EcsWorld::accsess_component_template(ComponentArray* array, EcsSizeT index) noexcept
    {
        al_assert(array->componentId == ComponentTypeInfo<T>::get_id());
        const EcsSizeT chunkIndex = index / EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK;
        const EcsSizeT inChunkIndex = index % EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK;
        return &reinterpret_cast<T*>(array->chunks[chunkIndex])[inChunkIndex];
    }

    inline uint8_t* EcsWorld::accsess_component(ComponentArray* array, EcsSizeT index) noexcept
    {
        const EcsSizeT chunkIndex = index / EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK;
        const EcsSizeT inChunkIndex = index % EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK;
        return &array->chunks[chunkIndex][inChunkIndex * component_info(array->componentId)->sizeBytes];
    }

    inline EntityHandle* EcsWorld::accsess_entity_handle(ArchetypeHandle handle, EcsSizeT index) noexcept
    {
        const EcsSizeT chunkIndex = index / EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK;
        const EcsSizeT inChunkIndex = index % EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK;
        return &archetype(handle)->entityHandlesChunks[chunkIndex][inChunkIndex];
    }

    inline EntityHandle* EcsWorld::accsess_entity_handle(Archetype* archetypePtr, EcsSizeT index) noexcept
    {
        const EcsSizeT chunkIndex = index / EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK;
        const EcsSizeT inChunkIndex = index % EngineConfig::NUMBER_OF_ELEMENTS_IN_ARCHETYPE_CHUNK;
        return &archetypePtr->entityHandlesChunks[chunkIndex][inChunkIndex];
    }

    EcsWorld::ComponentArray* EcsWorld::accsess_component_array(ArchetypeHandle handle, ComponentId componentId) noexcept
    {
        Archetype* archetypePtr = archetype(handle);
        return &archetypePtr->components[componentId];
    }

    inline bool EcsWorld::is_valid_subset(ComponentFlags subset, ComponentFlags superset) noexcept
    {
        return  ((subset.flags[0] & superset.flags[0]) == subset.flags[0]) &&
                ((subset.flags[1] & superset.flags[1]) == subset.flags[1]);
    }

    inline bool EcsWorld::is_valid_component_array(ComponentArray* array) noexcept
    {
        return array->componentId != 0;
    }
}
