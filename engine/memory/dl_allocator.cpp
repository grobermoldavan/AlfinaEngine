
// #include "dl_allocator.h"

// namespace al::engine
// {
//     [[nodiscard]] std::byte* DlAllocator::allocate(std::size_t memorySizeBytes) noexcept
//     {
//         std::byte* result = static_cast<std::byte*>(::dlmalloc(memorySizeBytes));
//         return result;
//     }

//     void DlAllocator::deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept
//     {
//         ::dlfree(ptr);
//     }
// }
