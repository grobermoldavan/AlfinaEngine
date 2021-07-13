#ifndef AL_IMAGE_2D_BASE_H
#define AL_IMAGE_2D_BASE_H

#include "engine/types.h"
#include "engine/math/math.h"
#include "texture_formats.h"

namespace al
{
    struct RendererBackend;

    enum struct TextureType : u8
    {
        _2D,
    };

    struct Texture
    {
        const u32_3 size;
        const TextureType type;
        const TextureFormat format;
    };

    struct TextureCreateInfo
    {
        u32_3 size;
        TextureType type;
        TextureFormat format;
    };

    struct TextureVtable
    {
        // Texture* (*create_from_file)(RendererBackend* backend, const char* filePath);
        Texture* (*create)(RendererBackend* backend, TextureCreateInfo* createInfo);
        void (*destroy)(RendererBackend* backend, Texture* texture);
    };
}

#endif
