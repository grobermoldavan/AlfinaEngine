#ifndef AL_THREAD_SAFE_QUEUE_H
#define AL_THREAD_SAFE_QUEUE_H

#include <cstddef>
#include <atomic>
#include <new> // for std::hardware_destructive_interference_size
#include <concepts>

#include "../non_copyable.h"
#include "../constexpr_functions.h"

namespace al
{
    // @NOTE : Implementation is taken from http://rsdn.org/forum/cpp/3730905.1
    template<std::copyable T>
    class ThreadSafeQueue : NonCopyable
    {
    public:
        struct Cell
        {
            std::atomic<std::size_t> sequence;
            T data;
        };

        // @NOTE : size must be a power of two
        ThreadSafeQueue(Cell* memory, std::size_t size) noexcept
            : buffer{ memory }
            , bufferMask{ size - 1}
        { }

        ~ThreadSafeQueue() = default;

        void initialize(std::size_t size) noexcept
        {
            for (std::size_t it = 0; it < size; it++)
            {
                buffer[it].sequence.store(it, std::memory_order_relaxed);
            }
            enqueuePos.store(0, std::memory_order_relaxed);
            dequeuePos.store(0, std::memory_order_relaxed);
        }

        bool enqueue(const T* data) noexcept
        {
            Cell* cell;
            // Load current enqueue position
            std::size_t pos = enqueuePos.load(std::memory_order_relaxed);
            while(true)
            {
                // Get current cell
                cell = &buffer[pos & bufferMask];
                // Load sequence of current cell
                std::size_t seq = cell->sequence.load(std::memory_order_acquire);
                std::intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
                // Cell is ready for write
                if (diff == 0)
                {
                    // Try to increment enqueue position
                    if (enqueuePos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        // If success, quit from the loop
                        break;
                    }
                    // Else try again
                }
                // Queue is full
                else if (diff < 0)
                {
                    return false;
                }
                // Cell was changed by some other thread
                else
                {
                    // Load current enqueue position and try again
                    pos = enqueuePos.load(std::memory_order_relaxed);
                }
            }
            
            // Write data
            cell->data = *data;
            // Update sequence
            cell->sequence.store(pos + 1, std::memory_order_release);
            return true;
        }

        bool dequeue(T* data) noexcept
        {
            Cell* cell;
            // Load current dequeue position
            std::size_t pos = dequeuePos.load(std::memory_order_relaxed);
            while(true)
            {
                // Get current cell
                cell = &buffer[pos & bufferMask];
                // Load sequence of current cell
                std::size_t seq = cell->sequence.load(std::memory_order_acquire);
                std::intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
                // Cell is ready for read
                if (diff == 0)
                {
                    // Try to increment dequeue position
                    if (dequeuePos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                    {
                        // If success, quit from the loop
                        break;
                    }
                    // Else try again
                }
                // Queue is empty
                else if (diff < 0)
                {
                    return false;
                }
                // Cell was changed by some other thread
                else
                {
                    // Load current dequeue position and try again
                    pos = dequeuePos.load(std::memory_order_relaxed);
                }
            }

            // Read data
            *data = cell->data;
            // Update sequence
            cell->sequence.store(pos + bufferMask + 1, std::memory_order_release);
            return true;
        }

    private:
        typedef std::byte CachelinePadding[std::hardware_destructive_interference_size];

        CachelinePadding            pad0;
        Cell * const                buffer;
        const std::size_t           bufferMask;
        CachelinePadding            pad1;
        std::atomic<std::size_t>    enqueuePos;
        CachelinePadding            pad2; 
        std::atomic<std::size_t>    dequeuePos;
        CachelinePadding            pad3;
    };

    template<std::copyable T, std::size_t Capacity>
    class StaticThreadSafeQueue : public ThreadSafeQueue<T>
    {
    public:
        StaticThreadSafeQueue()
            : storage{ }
            , ThreadSafeQueue{ storage, Capacity }
        {
            // We can't do the initialize logic in ThreadSafeQueue constructor
            // because storage memory can be cleared after this contructor
            initialize(Capacity);
        }

    private:
        Cell storage[Capacity];
    };
}

#endif