#ifndef AL_THREAD_SAFE_QUEUE_H
#define AL_THREAD_SAFE_QUEUE_H

#include "engine/types.h"
#include "engine/platform/platform_atomics.h"

namespace al
{
    // @NOTE : Implementation is taken from http://rsdn.org/forum/cpp/3730905.1
    template<typename T>
    struct ThreadSafeQueue
    {
        struct Cell
        {
            Atomic<u64> sequence;
            T data;
        };
        typedef u8 CachelinePadding[64];

        CachelinePadding    pad0;
        Cell*               buffer;
        u64                 bufferMask;
        CachelinePadding    pad1;
        Atomic<u64>         enqueuePos;
        CachelinePadding    pad2; 
        Atomic<u64>         dequeuePos;
        CachelinePadding    pad3;
    };

    // @NOTE : size must be a power of two
    template<typename T>
    void thread_safe_queue_construct(ThreadSafeQueue<T>* queue, typename ThreadSafeQueue<T>::Cell* memory, u64 size)
    {
        queue->buffer = memory;
        queue->bufferMask = size - 1;
        platform_atomic_64_bit_store(&queue->enqueuePos, u64(0), MemoryOrder::RELAXED);
        platform_atomic_64_bit_store(&queue->dequeuePos, u64(0), MemoryOrder::RELAXED);
        for (u64 it = 0; it < size; it++)
        {
            platform_atomic_64_bit_store(&queue->buffer[it].sequence, it, MemoryOrder::RELAXED);
        }
    }

    template<typename T>
    void thread_safe_queue_destruct(ThreadSafeQueue<T>* queue)
    {
        
    }

    template<typename T>
    bool thread_safe_queue_enqueue(ThreadSafeQueue<T>* queue, const T* data)
    {
        typename ThreadSafeQueue<T>::Cell* cell;
        // Load current enqueue position
        u64 pos = platform_atomic_64_bit_load(&queue->enqueuePos, MemoryOrder::RELAXED);
        while(true)
        {
            // Get current cell
            cell = &queue->buffer[pos & queue->bufferMask];
            // Load sequence of current cell
            u64 seq = platform_atomic_64_bit_load(&cell->sequence, MemoryOrder::ACQUIRE);
            std::intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos);
            // Cell is ready for write
            if (diff == 0)
            {
                // Try to increment enqueue position
                if (platform_atomic_64_bit_cas(&queue->enqueuePos, &pos, pos + 1, MemoryOrder::RELAXED))
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
                pos = platform_atomic_64_bit_load(&queue->enqueuePos, MemoryOrder::RELAXED);
            }
        }
        // Write data
        cell->data = *data;
        // Update sequence
        platform_atomic_64_bit_store(&cell->sequence, pos + 1, MemoryOrder::RELEASE);
        return true;
    }

    template<typename T>
    bool thread_safe_queue_dequeue(ThreadSafeQueue<T>* queue, T* data)
    {
        typename ThreadSafeQueue<T>::Cell* cell;
        // Load current dequeue position
        u64 pos = platform_atomic_64_bit_load(&queue->dequeuePos, MemoryOrder::RELAXED);
        while(true)
        {
            // Get current cell
            cell = &queue->buffer[pos & queue->bufferMask];
            // Load sequence of current cell
            u64 seq = platform_atomic_64_bit_load(&cell->sequence, MemoryOrder::ACQUIRE);
            std::intptr_t diff = static_cast<intptr_t>(seq) - static_cast<intptr_t>(pos + 1);
            // Cell is ready for read
            if (diff == 0)
            {
                // Try to increment dequeue position
                if (platform_atomic_64_bit_cas(&queue->dequeuePos, &pos, pos + 1, MemoryOrder::RELAXED))
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
                pos = platform_atomic_64_bit_load(&queue->dequeuePos, MemoryOrder::RELAXED);
            }
        }
        // Read data
        *data = cell->data;
        // Update sequence
        platform_atomic_64_bit_store(&cell->sequence, pos + queue->bufferMask + 1, MemoryOrder::RELEASE);
        return true;
    }
}

#endif
