#include "file_load.h"

namespace al::engine
{
    FileHandle sync_load(std::string_view name, AllocatorBase* allocator, FileLoadMode mode)
    {
        std::FILE* file = std::fopen(name.data(), LOAD_MODE_TO_STR[static_cast<int>(mode)]);   
        al_assert(file);

        std::size_t fileSize;
        std::fseek(file, 0, SEEK_END);
        fileSize = std::ftell(file);
        std::fseek(file, 0, SEEK_SET);
        std::byte* memory = allocator->allocate(fileSize + 1);
        al_assert(memory);

        std::size_t readSize = std::fread(memory, sizeof(std::byte), fileSize, file);
        al_assert(readSize == fileSize);

        std::fclose(file);
        memory[fileSize] = std::byte{ 0 };

        return
        {
            .size{ fileSize + 1 },
            .state{ FileHandle::State::LOADED },
            .memory{ memory }
        };
    }
}
