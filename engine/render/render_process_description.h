#ifndef AL_RENDERER_PROCESS_DESCRIPTION_H
#define AL_RENDERER_PROCESS_DESCRIPTION_H

#include "engine/types.h"
#include "engine/platform/platform.h"
#include "engine/utilities/utilities.h"

namespace al
{
    struct ImageAttachmentDescription
    {
        enum Format
        {
            DEPTH_32F,
            RGBA_8,
            RGB_32F,
        };
        Format format;
        u32 width;
        u32 height;
    };

    struct SampledAttachmentReference
    {
        uSize imageAttachmentIndex;
        const char* name; // set and binding will be retrieved from reflection data
    };

    struct OutputAttachmentReference
    {
        uSize imageAttachmentIndex;
        const char* name; // location will be retrieved from reflection data
    };

    struct DepthUsageInfo
    {
        uSize imageAttachmentIndex;
    };

    struct StageDependency
    {
        uSize stageIndex;
    };

    struct RenderStageDescription
    {
        PointerWithSize<PlatformFile> shaders;
        PointerWithSize<SampledAttachmentReference> sampledAttachments;
        PointerWithSize<OutputAttachmentReference> outputAttachments;
        DepthUsageInfo* depth;
        PointerWithSize<StageDependency> stageDependencies;
        const char* name;
    };

    struct RenderProcessDescription
    {
        PointerWithSize<ImageAttachmentDescription> imageAttachments;
        PointerWithSize<RenderStageDescription> stages;
        uSize resultImageAttachmentIndex;
    };
}

#endif
