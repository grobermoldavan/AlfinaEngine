#ifndef AL_COMMAND_BUFFER_H
#define AL_COMMAND_BUFFER_H

#include <cstddef>
#include <concepts>

#include "engine/debug/debug.h"

#include "utilities/function.h"
#include "utilities/concepts.h"
#include "utilities/thread_safe/thread_safe_only_growing_stack.h"

namespace al::engine
{
    template<al::pod Key>
    struct CommandDataBase
    {
        Key key;
    };

    template<al::pod CommandKey, std::derived_from<CommandDataBase<CommandKey>> CommandData, std::size_t BufferSize>
    class CommandBuffer
    {
    public:
        CommandBuffer() = default;

        ~CommandBuffer() noexcept
        {
            clear();
        }

        [[nodiscard]] CommandData* add_command(CommandKey key) noexcept
        {
            std::size_t index;
            CommandData* result = stack.get();
            al_assert(result);
            result->key = key;

            return result;
        }

        void clear() noexcept
        {
            stack.clear();
        }

        void sort() noexcept
        {
            // @TODO : sort
        }

        void for_each(Function<void(CommandData*)> func) noexcept
        {
            stack.for_each(func);
        }

        std::size_t size() const noexcept
        {
            return stack.size();
        }

    private:
        ThreadSafeOnlyGrowingStack<CommandData, BufferSize> stack;
    };
}

#endif
