#ifndef AL_THREAD_LOCAL_STORAGE_H
#define AL_THREAD_LOCAL_STORAGE_H

#include <cstring>

#include "engine/types.h"
#include "engine/memory/memory.h"
#include "engine/platform/platform_threads.h" // Can't just include platform.h because of circular dependencies. Sad!
#include "engine/platform/platform_atomics.h"

namespace al
{
    template<typename T, uSize Capacity>
    struct ThreadLocalStorage
    {
        T memory[Capacity];
        PlatformThreadId threadIds[Capacity];
        Atomic<uSize> size;
        void (*storageItemConstructor)(T*);
    };

    template<typename T, uSize Capacity>
    void tls_construct(ThreadLocalStorage<T, Capacity>* storage, void (*storageItemConstructor)(T*) = nullptr)
    {
        storage->storageItemConstructor = storageItemConstructor;
        storage->size = 0;
        std::memset(storage->memory, 0, Capacity * sizeof(T));
        std::memset(storage->threadIds, 0, Capacity * sizeof(PlatformThreadId));
    }

    template<typename T, uSize Capacity>
    void tls_destroy(ThreadLocalStorage<T, Capacity>* storage)
    {
        
    }

    template<typename T, uSize Capacity>
    T* tls_access(ThreadLocalStorage<T, Capacity>* storage)
    {
        PlatformThreadId currentThreadId = platform_get_current_thread_id();
        for (uSize it = 0; it < storage->size; it++)
        {
            if (storage->threadIds[it] == currentThreadId)
            {
                return &storage->memory[it];
            }
        }
        if (storage->size == Capacity)
        {
            return nullptr;
        }
        uSize expected = storage->size;
        uSize newSize = storage->size + 1;
        while (!platform_atomic_64_bit_cas(&storage->size, &expected, newSize, MemoryOrder::ACQUIRE_RELEASE))
        {
            newSize = expected + 1;
        }
        storage->threadIds[newSize - 1] = currentThreadId;
        T* tlsItem = &storage->memory[newSize - 1];
        if (storage->storageItemConstructor) storage->storageItemConstructor(tlsItem);
        return tlsItem;
    }
}

#endif
