
// #include "system_allocator.h"

// namespace al::engine
// {
//     [[nodiscard]] std::byte* SystemAllocator::allocate(std::size_t memorySizeBytes) noexcept
//     {
//         std::byte* result = static_cast<std::byte*>(::malloc(memorySizeBytes));
//         return result;
//     }

//     void SystemAllocator::deallocate(std::byte* ptr, std::size_t memorySizeBytes) noexcept
//     {
//         ::free(ptr);
//     }
// }
