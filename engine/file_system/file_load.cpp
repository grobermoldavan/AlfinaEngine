#include "file_load.h"

namespace al::engine
{
    FileHandle sync_load(const char* name, AllocatorBindings bindings, FileLoadMode mode)
    {
        std::FILE* file = std::fopen(name, LOAD_MODE_TO_STR[static_cast<int>(mode)]);   
        al_assert(file);
        std::size_t fileSize;
        std::fseek(file, 0, SEEK_END);
        fileSize = std::ftell(file);
        std::fseek(file, 0, SEEK_SET);
        void* memory = bindings.allocate(bindings.allocator, fileSize + 1);
        al_assert(memory);
        std::size_t readSize = std::fread(memory, sizeof(std::byte), fileSize, file);
        al_assert(readSize == fileSize);
        std::fclose(file);
        static_cast<uint8_t*>(memory)[fileSize] = 0;
        return
        {
            .size{ fileSize + 1 },
            .state{ FileHandle::State::LOADED },
            .memory{ memory }
        };
    }
}
