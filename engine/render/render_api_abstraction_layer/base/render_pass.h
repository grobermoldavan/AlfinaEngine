#ifndef AL_RENDER_PASS_H
#define AL_RENDER_PASS_H

#include "engine/types.h"
#include "enums.h"

namespace al
{
    struct RenderDevice;

    struct RenderPass
    {

    };

    struct RenderPassCreateInfo
    {
        static constexpr uSize MAX_COLOR_ATTACHMENTS = 32;
        enum struct DepthOp : u32
        {
            NOTHING,
            READ_WRITE,
            READ,
        };
        struct Attachment
        {
            TextureFormat format;
            AttachmentLoadOp loadOp;
            AttachmentStoreOp storeOp;
            u8 padding;
        };
        struct Subpass
        {
            u32 colorRefs;   // Bitmask. Each bit references attachment from colorAttachments array
            u32 inputRefs;   // Bitmask. Each bit references attachment from colorAttachments array
            u32 resolveRefs; // Bitmask. Each bit references attachment from colorAttachments array
            DepthOp depthOp; // DepthOp::NOTHING is depth attachment is not used in subpass
        };
        PointerWithSize<Subpass> subpasses;
        PointerWithSize<Attachment> colorAttachments;
        Attachment* depthStencilAttachment;
        RenderDevice* device;
    };
}

#endif
