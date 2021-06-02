#ifndef AL_FIXED_SIZE_BUFFER_H
#define AL_FIXED_SIZE_BUFFER_H

#include "engine/types.h"

namespace al
{
    template<uSize SizeBytes>
    struct FixedSizeBuffer
    {
        uSize bufferTop;
        u8 buffer[SizeBytes];        
    };

    template<uSize SizeBytes>
    void* fixed_size_buffer_allocate(FixedSizeBuffer<SizeBytes>* buffer, uSize sizeBytes)
    {
        if (SizeBytes - buffer->bufferTop < sizeBytes)
        {
            return nullptr;
        }
        void* result = &buffer->buffer[buffer->bufferTop];
        buffer->bufferTop += sizeBytes;
        return result;
    }

    template<typename T, uSize SizeBytes>
    T* fixed_size_buffer_allocate(FixedSizeBuffer<SizeBytes>* buffer, uSize amount = 1)
    {
        return (T*)fixed_size_buffer_allocate(buffer, sizeof(T) * amount);
    }

    template<uSize SizeBytes>
    void fixed_size_buffer_reset(FixedSizeBuffer<SizeBytes>* buffer)
    {
        buffer->bufferTop = 0;
    }
    
}

#endif