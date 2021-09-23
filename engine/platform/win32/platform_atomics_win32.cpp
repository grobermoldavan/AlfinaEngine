
#include "../platform_atomics.h"
#include "platform_win32_backend.h"

namespace al
{
    template<typename T>
    Result<T> platform_atomic_64_bit_increment(Atomic<T>* atomic)
    {
        dbg (if (!atomic) return err<T>("Value was a nullptr."));
        return ok<T>(InterlockedIncrement64((volatile s64*)&atomic->value));
    }

    template<typename T>
    Result<T> platform_atomic_64_bit_decrement(Atomic<T>* atomic)
    {
        dbg (if (!atomic) return err<T>("Value was a nullptr."));
        return ok<T>(InterlockedDecrement64((volatile s64*)&atomic->value));
    }

    template<typename T>
    Result<T> platform_atomic_64_bit_add(Atomic<T>* atomic, T other)
    {
        dbg (if (!atomic) return err<T>("Value was a nullptr."));
        return ok<T>(InterlockedAdd64((volatile s64*)&atomic->value, al_s64_bit_cast(&other)));
    }

    template<typename T>
    Result<T> platform_atomic_64_bit_load(Atomic<T>* atomic, MemoryOrder memoryOrder)
    {
        dbg (if (!atomic) return err<T>("Value was a nullptr."));
        dbg 
        (
            if (memoryOrder != MemoryOrder::RELAXED && 
                memoryOrder != MemoryOrder::CONSUME && 
                memoryOrder != MemoryOrder::ACQUIRE && 
                memoryOrder != MemoryOrder::SEQUENTIALLY_CONSISTENT) 
                return err<T>("Incorrect memory order. Expected values : RELAXED, CONSUME, ACQUIRE or SEQUENTIALLY_CONSISTENT.")
        );
        T loaded = atomic->value;
        if (memoryOrder == MemoryOrder::SEQUENTIALLY_CONSISTENT) MemoryBarrier();
        return ok<T>(loaded);
    }

    template<typename T>
    Result<T> platform_atomic_64_bit_store(Atomic<T>* atomic, T newValue, MemoryOrder memoryOrder)
    {
        dbg (if (!atomic) return err<T>("Value was a nullptr."));
        dbg 
        (
            if (memoryOrder != MemoryOrder::RELAXED && 
                memoryOrder != MemoryOrder::RELEASE && 
                memoryOrder != MemoryOrder::SEQUENTIALLY_CONSISTENT) 
                return err<T>("Incorrect memory order. Expected values : RELAXED, RELEASE or SEQUENTIALLY_CONSISTENT.")
        );
        atomic->value = newValue;
        if (memoryOrder == MemoryOrder::SEQUENTIALLY_CONSISTENT) MemoryBarrier();
        return ok<T>(newValue);
    }

    template<typename T>
    Result<bool> platform_atomic_64_bit_cas(Atomic<T>* atomic, T* expected, T newValue, MemoryOrder memoryOrder)
    {
        dbg (if (!atomic) return err<bool>("Value was a nullptr."));
        T val = *expected;
        s64 casResult = InterlockedCompareExchange64((volatile s64*)&atomic->value, bit_cast<s64>(newValue), bit_cast<s64>(*expected));
        *expected = bit_cast<T>(casResult);
        return ok<bool>(val == *expected);
    }
}