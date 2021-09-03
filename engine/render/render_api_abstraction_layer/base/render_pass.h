#ifndef AL_RENDER_PASS_H
#define AL_RENDER_PASS_H

/*
    Render pass consists of one or more subpasses, attachment descriptions and attachment utilization infos for each subpass.
*/

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
        static constexpr u32 UNUSED_ATTACHMENT = ~u32(0);
        static constexpr uSize MAX_ATTACHMENTS = 32;
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
            MultisamplingType samples;
        };
        struct Subpass
        {
            PointerWithSize<u32> colorRefs;     // each value references colorAttachments array
            PointerWithSize<u32> inputRefs;     // each value references colorAttachments array
            PointerWithSize<u32> resolveRefs;   // each value references colorAttachments array
            DepthOp depthOp;                    // DepthOp::NOTHING is depth attachment is not used in subpass
        };
        PointerWithSize<Subpass> subpasses;
        PointerWithSize<Attachment> colorAttachments;
        Attachment* depthStencilAttachment;
        RenderDevice* device;
    };
}

#endif
