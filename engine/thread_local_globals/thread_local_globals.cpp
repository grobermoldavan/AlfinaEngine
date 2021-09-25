
#include "thread_local_globals.h"
#include "engine/utilities/thread_local_storage.h"
#include "engine/platform/platform_atomics.h"

namespace al
{
    ThreadLocalStorage<ApplicationGlobals*, 1024> globalsStorage;
    Atomic<bool> isConstructed { .value = false };
    Atomic<bool> isConstructing { .value = false };

    void thread_local_globals_register(ApplicationGlobals* globals)
    {
        if (!platform_atomic_64_bit_load(&isConstructed, MemoryOrder::ACQUIRE))
        {
            bool expected = false;
            if (platform_atomic_64_bit_cas(&isConstructing, &expected, true, MemoryOrder::ACQUIRE_RELEASE))
            {
                tls_construct(&globalsStorage);
                platform_atomic_64_bit_store(&isConstructed, true, MemoryOrder::RELEASE);
                platform_atomic_64_bit_store(&isConstructing, false, MemoryOrder::RELEASE);
            }
            else
            {
                while (platform_atomic_64_bit_load(&isConstructing, MemoryOrder::ACQUIRE))
                {
                    platform_thread_yield();
                }
            }
        }
        *tls_access(&globalsStorage) = globals;
    }

    ApplicationGlobals* thread_local_globals_access()
    {
        return *tls_access(&globalsStorage);
    }
}