#ifndef AL_RENDERER_PROCESS_DESCRIPTION_H
#define AL_RENDERER_PROCESS_DESCRIPTION_H

#include "engine/types.h"
#include "engine/platform/platform.h"

namespace al
{
    template<typename T>
    struct PointerWithSize
    {
        T* ptr;
        uSize size;
    };

    struct ImageAttachment
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

    struct RenderStage
    {
        PointerWithSize<PlatformFile> shaders;
        PointerWithSize<SampledAttachmentReference> sampledAttachments;
        PointerWithSize<OutputAttachmentReference> outputAttachments;
        DepthUsageInfo* depth;
        PointerWithSize<StageDependency> stageDependencies;
    };

    struct RenderProcessDescription
    {
        PointerWithSize<ImageAttachment> imageAttachments;
        PointerWithSize<RenderStage> stages;
        uSize resultImageAttachmentIndex;
    };
}

#endif
