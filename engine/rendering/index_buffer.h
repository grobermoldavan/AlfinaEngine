#ifndef AL_INDEX_BUFFER_H
#define AL_INDEX_BUFFER_H

#include <cstddef>

namespace al::engine
{
    class IndexBuffer
    {
    public:
        virtual ~IndexBuffer() = default;

        virtual         void            bind        ()  const noexcept = 0;
        virtual         void            unbind      ()  const noexcept = 0;
        virtual const   std::size_t     get_count   ()  const noexcept = 0;
    };
}

#endif
