#ifndef AL_COMMAND_BUFFER_H
#define AL_COMMAND_BUFFER_H

#include <cstdint>
#include <atomic>

#include "engine/debug/debug.h"

#include "utilities/function.h"

namespace al::engine
{
    template<typename CommandKey, typename CommandData, std::size_t BufferSize>
    class CommandBuffer
    {
    public:
        CommandData* add_command(CommandKey key) noexcept
        {
            CommandData* result = nullptr;
            std::size_t index;
            bool allocationResult = allocate_buffer(&index);
            al_assert(allocationResult);
            result = data[index];
            return result;
        }

        void clear() noexcept
        {
            bufferPtr = 0;
        }

        void sort() noexcept
        {
            // @TODO : sort
        }

        void for_each(Function<void(CommandData*)> func)
        {
            for (std::size_t it = 0; it < bufferPtr; it++)
            {
                func(&data[it]);
            }
        }

    private:
        CommandKey keys[BufferSize];
        CommandData data[BufferSize];
        std::atomic<std::size_t> bufferPtr;

        bool allocate_buffer(std::size_t* value) noexcept
        {
            bool result = false;

            while (true)
            {
                std::size_t currentTop = bufferPtr;
                if (currentTop == (BufferSize - 1))
                {
                    break;
                }

                const bool casResult = bufferPtr.compare_exchange_strong(currentTop, currentTop + 1);
                if (casResult)
                {
                    *value = currentTop;
                    result = true;
                    break;
                }
            }

            return result;
        }
    };
}

#endif
