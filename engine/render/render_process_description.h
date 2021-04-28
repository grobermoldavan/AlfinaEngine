#ifndef AL_RENDERER_PIPELINE_DESCRIPTION_H
#define AL_RENDERER_PIPELINE_DESCRIPTION_H

#include "engine/types.h"
#include "engine/platform/platform.h"

namespace al
{
    struct AttachmentDescription
    {
        enum Format : u32
        {
            FORMAT_RGBA_8,
            FORMAT_RGBA_32F,
        };
        enum Flags : u32
        {
            FLAG_FRAMEBUFFER_SIZE_EQUALS_WINDOW_SIZE = 1 << 0,   // Framebuffer will be automatically resized to window size
            FLAG_SCREEN_FRAMEBUFFER                  = 1 << 1,   // This is window screen
        };
        uSize   width;
        uSize   height;
        Format  format;
        u32     flags;
        // @TODO :  Add an option to use double/triple/n-buffering
    };

    constexpr AttachmentDescription SCREEN_FRAMEBUFFER_DESC
    {
        .width  = { },
        .height = { },
        .format = { },
        .flags  = AttachmentDescription::FLAG_SCREEN_FRAMEBUFFER | AttachmentDescription::FLAG_FRAMEBUFFER_SIZE_EQUALS_WINDOW_SIZE,
    };

    struct AttachmentDependency
    {
        enum Flags : u32
        {
            FLAG_USE_AS_SAMPLER = 1 << 0,    // Attachment will be used as sampler2d, otherwise it will be used as subpassInput
        };
        uSize   attachmentIndex; // idex into RenderConfiguration::framebuffers array
        u32     flags;
    };

    struct RenderStageDescription
    {
        enum Flags : u32
        {
            FLAG_PASS_GEOMETRY   = 1 << 0,   // All geometry submitted for drawing will be passed to vertex shader
        };
        PlatformFile            vertexShader;
        PlatformFile            fragmentShader;
        uSize                   inAttachmentsSize;
        AttachmentDependency*   inAttachments;
        uSize                   outAttachmentsSize;
        AttachmentDependency*   outAttachments;
        u32                     flags;
        // @TODO :  Add an option to filter geometry by some kind of user-defined tag?
        //          If render pass doesn't need all geometry, but only some specific types of it
    };

    struct RenderProcessDescription
    {
        uSize                   attachmentsSize;
        AttachmentDescription*  attachments;
        uSize                   renderStagesSize;
        RenderStageDescription* renderStages;
    };
}

#endif
