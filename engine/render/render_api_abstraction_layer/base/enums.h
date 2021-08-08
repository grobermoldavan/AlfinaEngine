#ifndef AL_TEXTURE_FORMATS_H
#define AL_TEXTURE_FORMATS_H

#include "engine/types.h"

namespace al
{
    enum struct TextureFormat : u8
    {
        DEPTH_STENCIL,
        SWAP_CHAIN,
        RGBA_8,
        RGBA_32F,
    };

    enum struct AttachmentLoadOp : u8
    {
        NOTHING,
        CLEAR,
        LOAD,
    };

    enum struct AttachmentStoreOp : u8
    {
        NOTHING,
        STORE,
    };
}

#endif
