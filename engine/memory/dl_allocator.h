#ifndef AL_DL_ALLOCATOR
#define AL_DL_ALLOCATOR

#include "allocator_base.h"

#ifdef _MSC_VER 
#   pragma warning(disable : 4005)
#   include "engine/3d_party_libs/dlmalloc/dlmalloc.c"
#   pragma warning(default : 4005)
#else
#   include "engine/3d_party_libs/dlmalloc/dlmalloc.c"
#endif

// @NOTE :  This allocator is not thread-safe

namespace al::engine
{
    class DlAllocator : public AllocatorBase
    {
        virtual [[nodiscard]] std::byte* allocate(std::size_t memorySizeBytes) noexcept override;
        virtual void deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept override;
    };
}

#endif
