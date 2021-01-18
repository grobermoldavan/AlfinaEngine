
#include "memory_common.h"

namespace al::engine
{
    template<typename T>
    T* align_pointer(T* ptr) noexcept
    {
        uintptr_t uintPtr = reinterpret_cast<uintptr_t>(ptr);
        uint64_t diff = uintPtr & (EngineConfig::DEFAULT_MEMORY_ALIGNMENT - 1);
        if (diff != 0)
        {
            diff = EngineConfig::DEFAULT_MEMORY_ALIGNMENT - diff;
        }
        uint8_t* alignedPtr = reinterpret_cast<uint8_t*>(ptr) + diff;
        return reinterpret_cast<T*>(alignedPtr);
    }
}
