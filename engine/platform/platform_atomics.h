#ifndef AL_PLATFORM_ATOMICS_H
#define AL_PLATFORM_ATOMICS_H

#include "engine/types.h"
#include "engine/result.h"
#include "engine/utilities/bits.h"

namespace al
{
    enum struct MemoryOrder
    {
        RELAXED,
        CONSUME,
        ACQUIRE,
        RELEASE,
        ACQUIRE_RELEASE,
        SEQUENTIALLY_CONSISTENT
    };

    template<typename T> concept max_size_64 = sizeof(T) <= 8;
    template<typename T> concept size_less_64 = sizeof(T) < 8;
    template<typename T> concept size_64 = !size_less_64<T> && sizeof(T) == 8;

    template<typename T>
    struct Atomic;

    template<typename T> Result<T> platform_atomic_64_bit_increment  (Atomic<T>* atomic);
    template<typename T> Result<T> platform_atomic_64_bit_decrement  (Atomic<T>* atomic);
    template<typename T> Result<T> platform_atomic_64_bit_add        (Atomic<T>* atomic, T other);
    template<typename T> Result<T> platform_atomic_64_bit_load       (Atomic<T>* atomic, MemoryOrder memoryOrder);
    template<typename T> Result<T> platform_atomic_64_bit_store      (Atomic<T>* atomic, T newValue, MemoryOrder memoryOrder);
    template<typename T> Result<bool> platform_atomic_64_bit_cas     (Atomic<T>* atomic, T* expected, T newValue, MemoryOrder memoryOrder);

    template<size_less_64 T>
    struct Atomic<T>
    {
        volatile T value;
        u8 __padding[8 - sizeof(T)];

        operator T()
        {
            return unwrap(platform_atomic_64_bit_load(this, MemoryOrder::RELAXED));
        }
        Atomic<T>& operator = (T newValue)
        {
            platform_atomic_64_bit_store(this, newValue, MemoryOrder::RELAXED);
            return *this;
        }
    };

    template<size_64 T>
    struct Atomic<T>
    {
        volatile T value;

        operator T()
        {
            return unwrap(platform_atomic_64_bit_load(this, MemoryOrder::RELAXED));
        }
        Atomic<T>& operator = (T newValue)
        {
            platform_atomic_64_bit_store(this, newValue, MemoryOrder::RELAXED);
            return *this;
        }
    };
}

#endif
