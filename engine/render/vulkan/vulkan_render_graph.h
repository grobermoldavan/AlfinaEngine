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
        //
        // VulkanRenderGraph::ImageAttachment struct must have all information
        // neccessary for vulkan backend to create actual vkImages and other stuff
        //
        struct ImageAttachment
        {
            VkExtent3D extent;
            VkFormat format;
            VkImageAspectFlags aspect;
            VkImageUsageFlags usage;
            VkImageType type;
        };
        //
        // VulkanRenderGraph::DescriptorSet struct must have all information
        // neccessary for vulkan backend to create all descriptor sets used in a
        // given RenderStage. Currently all descriptor sets are unoptimized -
        // if some shaders of RenderStage use same data at same set and same location
        // it will still be duplicated.
        //
        struct DescriptorSet
        {
            struct Binding
            {
                VkDescriptorSetLayoutBinding vkDescription;
            };
            uSize set;
            PointerWithSize<Binding> bindings;
        };

        struct RenderPass
        {
            struct Attachment
            {

            };
            VkRenderPassCreateInfo createInfo;
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
