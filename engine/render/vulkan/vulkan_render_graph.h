#ifndef AL_VULKAN_RENDER_GRAPH_H
#define AL_VULKAN_RENDER_GRAPH_H

#include "../render_process_description.h"
#include "vulkan_base.h"
#include "engine/utilities/utilities.h"

namespace al::vulkan
{
    //
    // VulkanRenderGraph struct contains all information needed to create
    // correctly-functional vulkan render primitives and to organize render
    // of a single frame.
    //
    struct VulkanRenderGraph
    {
        static constexpr uSize BUFFER_SIZE = 4096;
        struct ImageAttachment
        {
            VkExtent3D extent;
            VkFormat format;
            VkImageAspectFlags aspect;
            VkImageUsageFlags usage;
            VkImageType type;
        };
        struct DescriptorSetBinding
        {
            uSize binding;
            uSize sizeBytes;
            VkDescriptorType descriptorType;
            VkShaderStageFlags stageFlags;
        };
        struct DescriptorSet
        {
            uSize set;
            PointerWithSize<DescriptorSetBinding> bindings;
        };
        //
        // VulkanRenderGraph::RenderPass basically contains all struff required
        // for vkCreateRenderPass call
        //
        struct RenderPass
        {
            PointerWithSize<VkAttachmentDescription> attachmentDescriptions;
            PointerWithSize<VkAttachmentReference> colorAttachmentReferences;
            VkAttachmentReference* depthAttachmentReference;
            VkSubpassDescription subpassDescription;
            VkSubpassDependency subpassDependencies[2];
            VkRenderPassCreateInfo createInfo;
        };
        struct Framebuffer
        {
            PointerWithSize<uSize> imageAttachmentReferences;
        };
        struct RenderPipelineShaderStage
        {
            PlatformFile bytecode;
            VkShaderStageFlagBits stage;
            const char* entryPoint;
        };
        struct RenderPipeline
        {
            PointerWithSize<RenderPipelineShaderStage> shaderStages;
            PointerWithSize<VkPushConstantRange> pushConstants;
        };
        //
        // VulkanRenderGraph::RenderStage struct must have all information
        // neccessary for vulkan backend to create a single render pass,
        // single render pipeline, pass framebuffers, descriptor sets and
        // to use it in rendering.
        //
        struct RenderStage
        {
            PointerWithSize<DescriptorSet> descriptorSets;
            RenderPass renderPass;
            Framebuffer framebuffer;
            RenderPipeline renderPipeline;
            PointerWithSize<uSize> stageDependencies;
        };
        FixedSizeBuffer<BUFFER_SIZE> buffer;
        //
        // All images used for rendering are declared up-front by the user in RenderProcessDescription.
        //
        PointerWithSize<ImageAttachment> imageAttachments;
        //
        // All render stages are declared up-front by the user in RenderProcessDescription.
        //
        PointerWithSize<RenderStage> renderStages;
    };

    void construct_vulkan_render_graph(VulkanRenderGraph* graph, RenderProcessDescription* renderProcessDescription, AllocatorBindings bindings);
}

#endif
