#ifndef AL_ALLOCATOR_BASE_H
#define AL_ALLOCATOR_BASE_H

#include <cstddef>

namespace al::engine
{
    class AllocatorBase 
    {
    public:
        virtual [[nodiscard]] std::byte* allocate(std::size_t memorySizeBytes) noexcept = 0;
    };
}

#endif
