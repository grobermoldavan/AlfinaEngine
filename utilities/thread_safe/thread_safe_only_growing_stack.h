#ifndef AL_THREAD_SAFE_ONLY_GROWING_STACK_H
#define AL_THREAD_SAFE_ONLY_GROWING_STACK_H

#include <cstddef>
#include <atomic>
#include <type_traits>

#include "utilities/function.h"

namespace al
{
    template<typename T, std::size_t BufferSize>
    class ThreadSafeOnlyGrowingStack
    {
    public:
        ThreadSafeOnlyGrowingStack() = default;

        ~ThreadSafeOnlyGrowingStack() noexcept
        {
            clear();
        }

        T* push(const T& value) noexcept
        {
            std::size_t index;
            bool result = incremet_buffer_ptr(&index);
            if (result)
            {
                memory[index] = value;
                return &memory[index];
            }
            return nullptr;
        }

        T* get() noexcept
        {
            std::size_t index;
            bool result = incremet_buffer_ptr(&index);
            if (result)
            {
                return &memory[index];
            }
            return nullptr;
        }

        void clear() noexcept
        {
            destruct_current_objects();
            bufferPtr = 0;
        }

        std::size_t size() const noexcept
        {
            return bufferPtr;
        }

        T* data() noexcept
        {
            return memory;
        }

        void for_each(Function<void(T*)> func) noexcept
        {
            for (std::size_t it = 0; it < bufferPtr; it++)
            {
                func(&memory[it]);
            }
        }

    private:
        T memory[BufferSize];
        std::atomic<std::size_t> bufferPtr;

        bool incremet_buffer_ptr(std::size_t* value) noexcept
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

        void destruct_current_objects() noexcept
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
            {
                for (std::size_t it = 0; it < bufferPtr; it++)
                {
                    memory[it].~T();
                }
            }
        }
    };
}

#endif
