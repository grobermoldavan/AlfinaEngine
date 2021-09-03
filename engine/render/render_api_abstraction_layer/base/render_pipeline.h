#ifndef AL_RENDER_PIPELINE_H
#define AL_RENDER_PIPELINE_H

/*
    Render pipeline contains info about whole state of renderer. Unlike vulkan, this repnder pipeline also contains
    information about descriptor sets used in the shaders. Those descriptor sets can be allocated using pipeline instances.
*/

#include "engine/types.h"

namespace al
{
    struct RenderDevice;
    struct RenderPass;
    struct RenderProgram;

    struct DescriptorSet
    {

    };

    struct RenderPipeline
    {

    };

    struct StencilOpState
    {
        u32 compareMask;
        u32 writeMask;
        u32 reference;
        StencilOp failOp;
        StencilOp passOp;
        StencilOp depthFailOp;
        CompareOp compareOp;
    };

    struct DepthTestState
    {
        f32 minDepthBounds;
        f32 maxDepthBounds;
        bool isTestEnabled;
        bool isWriteEnabled;
        bool isBoundsTestEnabled;
        CompareOp compareOp;
    };

    struct GraphicsRenderPipelineCreateInfo
    {
        RenderDevice* device;
        RenderPass* pass;
        RenderProgram* vertexProgram;
        RenderProgram* fragmentProgram;
        StencilOpState* frontStencilOpState;
        StencilOpState* backStencilOpState;
        DepthTestState* depthTestState;
        uSize subpassIndex;
        PipelinePoligonMode poligonMode;
        PipelineCullMode cullMode;
        PipelineFrontFace frontFace;
        MultisamplingType multisamplingType;
    };
}

#endif

 