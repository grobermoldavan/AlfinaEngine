#ifndef AL_THREAD_LOCAL_STORAGE_H
#define AL_THREAD_LOCAL_STORAGE_H

#include <cstring>

#include "engine/types.h"
#include "engine/memory/memory.h"
#include "engine/platform/platform_threads.h" // Can't just include platform.h because of circular dependencies. Sad!

namespace al
{
    template<typename T>
    struct ThreadLocalStorage
    {
        static constexpr uSize DEFAULT_CAPACITY = 8;
        AllocatorBindings bindings;
        PlatformThreadId* threadIds;
        T* memory;
        uSize size;
        uSize capacity;
    };

    template<typename T>
    void tls_construct(ThreadLocalStorage<T>* storage, AllocatorBindings bindings)
    {
        storage->bindings = bindings;
        storage->capacity = ThreadLocalStorage<T>::DEFAULT_CAPACITY;
        storage->size = 0;
        storage->memory = static_cast<T*>(allocate(&storage->bindings, storage->capacity * sizeof(T)));
        std::memset(storage->memory, 0, storage->capacity * sizeof(T));
        storage->threadIds = static_cast<PlatformThreadId*>(allocate(&storage->bindings, storage->capacity * sizeof(PlatformThreadId)));
    }

    template<typename T>
    void tls_destroy(ThreadLocalStorage<T>* storage)
    {
        deallocate(&storage->bindings, storage->memory, storage->capacity * sizeof(T));
    }

    template<typename T>
    T* tls_access(ThreadLocalStorage<T>* storage)
    {
        PlatformThreadId currentThreadId = platform_get_current_thread_id();
        for (uSize it = 0; it < storage->size; it++)
        {
            if (storage->threadIds[it] == currentThreadId)
            {
                return &storage->memory[it];
            }
        }
        if (storage->size == storage->capacity)
        {
            uSize newCapacity = storage->capacity * 2;
            T* newMemory = static_cast<T*>(allocate(&storage->bindings, newCapacity * sizeof(T)));
            PlatformThreadId* newThreadIds = static_cast<PlatformThreadId*>(allocate(&storage->bindings, newCapacity * sizeof(PlatformThreadId)));
            std::memcpy(newThreadIds, storage->threadIds, storage->capacity * sizeof(PlatformThreadId));
            std::memcpy(newMemory, storage->memory, storage->capacity * sizeof(T));
            std::memset(newMemory + storage->capacity, 0, (newCapacity - storage->capacity) * sizeof(T));
            deallocate(&storage->bindings, storage->memory, storage->capacity * sizeof(T));
            deallocate(&storage->bindings, storage->threadIds, storage->capacity * sizeof(PlatformThreadId));
            storage->capacity = newCapacity;
            storage->memory = newMemory;
            storage->threadIds = newThreadIds;
        }
        uSize newPosition = storage->size++;
        storage->threadIds[newPosition] = currentThreadId;
        return &storage->memory[newPosition];
    }
}

#endif
