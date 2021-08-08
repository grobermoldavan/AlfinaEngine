
/*
    Render pass creation code is based on https://github.com/Themaister/Granite/blob/master/vulkan/render_pass.cpp

    vulkan_render_pass_create function automatically sets all neccessary attachment layout changes, preserve attachments,
    subpasses and subpass dependencies.
*/

#include "render_pass_vulkan.h"
#include "render_device_vulkan.h"

namespace al
{
    struct SubpassIntermediateData
    {
        Array<VkAttachmentReference> colorAttachmentRefs;
        Array<VkAttachmentReference> inputAttachmentRefs;
        Array<VkAttachmentReference> resolveAttachmentRefs;
        Array<u32> preserveAttachmentRefs;
        VkAttachmentReference depthStencilReference;
    };

    VkAttachmentReference* vulkan_find_attachment_reference(Array<VkAttachmentReference> refs, u32 attachmentIndex)
    {
        for (al_iterator(it, refs))
        {
            if (get(it)->attachment == attachmentIndex)
            {
                return get(it);
            }
        }
        return nullptr;
    }

    RenderPass* vulkan_render_pass_create(RenderPassCreateInfo* createInfo)
    {
        #define al_is_bit_set(mask, bit) ((mask >> (bit)) & 1)
        #define al_for_each_set_bit(mask, it) for (u32 it = 0; it < (sizeof(mask) * 8); it++) if (al_is_bit_set(mask, it))
        #define al_for_each_bit(mask, it) for (u32 it = 0; it < (sizeof(mask) * 8); it++)

        using BitMask = u32;

        auto countBits = [](BitMask mask) -> u32
        {
            u32 count = 0;
            al_for_each_set_bit(mask, it) count += 1;
            return count;
        };

        RenderDeviceVulkan* device = (RenderDeviceVulkan*)createInfo->device;
        RenderPassVulkan* pass = allocate<RenderPassVulkan>(&device->memoryManager.cpu_persistentAllocator);

        const bool isStencilSupported = device->gpu.flags & VulkanGpu::HAS_STENCIL;
        const bool hasDepthStencilAttachment = createInfo->depthStencilAttachment != nullptr;

        Array<VkAttachmentDescription> renderPassAttachmentDescriptions;
        array_construct(&renderPassAttachmentDescriptions, &device->memoryManager.cpu_frameAllocator, createInfo->colorAttachments.size + (hasDepthStencilAttachment ? 1 : 0));
        defer(array_destruct(&renderPassAttachmentDescriptions));

        al_vk_assert(renderPassAttachmentDescriptions.size <= 32);

        //
        // Attachments
        //

        if (hasDepthStencilAttachment)
        {
            renderPassAttachmentDescriptions[renderPassAttachmentDescriptions.size - 1] =
            {
                .flags          = 0,
                .format         = device->gpu.depthStencilFormat,
                .samples        = VK_SAMPLE_COUNT_1_BIT,
                .loadOp         = utils::to_vk_load_op(createInfo->depthStencilAttachment->loadOp),
                .storeOp        = utils::to_vk_store_op(createInfo->depthStencilAttachment->storeOp),
                .stencilLoadOp  = isStencilSupported ? utils::to_vk_load_op(createInfo->depthStencilAttachment->loadOp) : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = isStencilSupported ? utils::to_vk_store_op(createInfo->depthStencilAttachment->storeOp) : VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = isStencilSupported ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                .finalLayout    = VK_IMAGE_LAYOUT_UNDEFINED, // will be filled later
            };
        }
        for (al_iterator(it, createInfo->colorAttachments))
        {
            if (get(it)->format == TextureFormat::DEPTH_STENCIL)
            {
                al_vk_assert_fail("todo");
            }
            else 
            {
                const bool isSwapChainImage = get(it)->format == TextureFormat::SWAP_CHAIN;
                renderPassAttachmentDescriptions[to_index(it)] =
                {
                    .flags          = 0,
                    .format         = isSwapChainImage ? device->swapChain.surfaceFormat.format : utils::to_vk_format(get(it)->format),
                    .samples        = VK_SAMPLE_COUNT_1_BIT,
                    .loadOp         = utils::to_vk_load_op(get(it)->loadOp),
                    .storeOp        = utils::to_vk_store_op(get(it)->storeOp),
                    .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                    .initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    .finalLayout    = VK_IMAGE_LAYOUT_UNDEFINED, // will be filled later
                };
            }
        }

        //
        // Subpasses
        //

        Array<SubpassIntermediateData> subpassIntermediateDatas;
        Array<VkSubpassDescription> subpassDescriptions;
        array_construct(&subpassIntermediateDatas, &device->memoryManager.cpu_frameAllocator, createInfo->subpasses.size);
        array_construct(&subpassDescriptions, &device->memoryManager.cpu_frameAllocator, createInfo->subpasses.size);
        defer
        (
            for (al_iterator(it, subpassIntermediateDatas))
            {
                if (get(it)->colorAttachmentRefs.size) array_destruct(&get(it)->colorAttachmentRefs);
                if (get(it)->inputAttachmentRefs.size) array_destruct(&get(it)->inputAttachmentRefs);
                if (get(it)->resolveAttachmentRefs.size) array_destruct(&get(it)->resolveAttachmentRefs);
                if (get(it)->preserveAttachmentRefs.size) array_destruct(&get(it)->preserveAttachmentRefs);
            }
            array_destruct(&subpassIntermediateDatas);
        );
        defer(array_destruct(&subpassDescriptions));

        // globalUsedAttachmentsMask & 1 << attachmentIndex == 1 if attachment was already used in the render pass
        BitMask globalUsedAttachmentsMask = 0;
        
        for (al_iterator(it, createInfo->subpasses))
        {
            RenderPassCreateInfo::Subpass* subpassCreateInfo = get(it);
            SubpassIntermediateData* subpassIntermediateData = &subpassIntermediateDatas[to_index(it)];
            const u32 colorRefsSize = countBits(subpassCreateInfo->colorRefs);
            const u32 inputRefsSize = countBits(subpassCreateInfo->inputRefs);
            const u32 resolveRefsSize = colorRefsSize;
            if (colorRefsSize) array_construct(&subpassIntermediateData->colorAttachmentRefs, &device->memoryManager.cpu_frameAllocator, colorRefsSize);
            if (inputRefsSize) array_construct(&subpassIntermediateData->inputAttachmentRefs, &device->memoryManager.cpu_frameAllocator, inputRefsSize);
            if (resolveRefsSize) array_construct(&subpassIntermediateData->resolveAttachmentRefs, &device->memoryManager.cpu_frameAllocator, resolveRefsSize);
            // localUsedAttachmentsMask & 1 << attachmentIndex == 1 if attachment was already used in the subpass
            BitMask localUsedAttachmentsMask = 0;
            //
            // Fill color, input and resolve attachments
            //
            // References layouts will be filled later
            uSize refCounter = 0;
            al_for_each_set_bit(subpassCreateInfo->colorRefs, refIt)
            {
                subpassIntermediateData->colorAttachmentRefs[refCounter] = { refIt, VK_IMAGE_LAYOUT_UNDEFINED };
                subpassIntermediateData->resolveAttachmentRefs[refCounter] =
                {
                    al_is_bit_set(subpassCreateInfo->resolveRefs, refIt) ? refIt : VK_ATTACHMENT_UNUSED,
                    VK_IMAGE_LAYOUT_UNDEFINED
                };
                globalUsedAttachmentsMask |= BitMask(1) << refIt;
                localUsedAttachmentsMask |= BitMask(1) << refIt;
                refCounter += 1;
            }
            refCounter = 0;
            al_for_each_set_bit(subpassCreateInfo->inputRefs, refIt)
            {
                subpassIntermediateData->inputAttachmentRefs[refCounter] = { refIt, VK_IMAGE_LAYOUT_UNDEFINED };
                globalUsedAttachmentsMask |= BitMask(1) << refIt;
                localUsedAttachmentsMask |= BitMask(1) << refIt;
                refCounter += 1;
            }
            //
            // Fill depth stencil ref
            //
            switch (subpassCreateInfo->depthOp)
            {
                case RenderPassCreateInfo::DepthOp::NOTHING:
                {
                    subpassIntermediateData->depthStencilReference = { VK_ATTACHMENT_UNUSED, VK_IMAGE_LAYOUT_UNDEFINED };
                } break;
                case RenderPassCreateInfo::DepthOp::READ_WRITE:
                {
                    subpassIntermediateData->depthStencilReference = { u32(renderPassAttachmentDescriptions.size) - 1, VK_IMAGE_LAYOUT_UNDEFINED };
                    globalUsedAttachmentsMask |= BitMask(1) << (renderPassAttachmentDescriptions.size - 1);
                    localUsedAttachmentsMask |= BitMask(1) << (renderPassAttachmentDescriptions.size - 1);
                } break;
                case RenderPassCreateInfo::DepthOp::READ:
                {
                    subpassIntermediateData->depthStencilReference =
                    {
                        VK_ATTACHMENT_UNUSED,
                        hasDepthStencilAttachment
                            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                            : VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL
                    };
                    globalUsedAttachmentsMask |= BitMask(1) << (renderPassAttachmentDescriptions.size - 1);
                    localUsedAttachmentsMask |= BitMask(1) << (renderPassAttachmentDescriptions.size - 1);
                } break;
            }
            //
            // Fill preserve info
            //
            u32 preserveRefsSize = 0;
            BitMask preserveRefsMask = 0;
            for (al_iterator(attachmentIt, renderPassAttachmentDescriptions))
            {
                u32 attachmentIndex = u32(to_index(attachmentIt));
                // If attachment was used before, but current subpass is not using it, we preserve it
                if (al_is_bit_set(globalUsedAttachmentsMask, attachmentIndex) && !al_is_bit_set(localUsedAttachmentsMask, attachmentIndex))
                {
                    preserveRefsSize += 1;
                    preserveRefsMask |= BitMask(1) << attachmentIndex;
                }
            }
            if (preserveRefsSize) array_construct(&subpassIntermediateData->preserveAttachmentRefs, &device->memoryManager.cpu_frameAllocator, preserveRefsSize);
            refCounter = 0;
            al_for_each_set_bit(preserveRefsMask, preserveIt)
            {
                subpassIntermediateData->preserveAttachmentRefs[refCounter++] = preserveIt;
            }
            //
            // Setup the description
            //
            VkAttachmentReference* depthRef = nullptr;
            if (subpassIntermediateData->depthStencilReference.attachment != VK_ATTACHMENT_UNUSED)
            {
                depthRef = &subpassIntermediateData->depthStencilReference;
            }
            subpassDescriptions[to_index(it)] =
            {
                .flags                      = 0,
                .pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount       = inputRefsSize,
                .pInputAttachments          = inputRefsSize ? subpassIntermediateData->inputAttachmentRefs.memory : nullptr,
                .colorAttachmentCount       = colorRefsSize,
                .pColorAttachments          = colorRefsSize ? subpassIntermediateData->colorAttachmentRefs.memory : nullptr,
                .pResolveAttachments        = resolveRefsSize ? subpassIntermediateData->resolveAttachmentRefs.memory : nullptr,
                .pDepthStencilAttachment    = depthRef,
                .preserveAttachmentCount    = preserveRefsSize,
                .pPreserveAttachments       = preserveRefsSize ? subpassIntermediateData->preserveAttachmentRefs.memory : nullptr,
            };
        }

        //
        // Dependencies and layouts
        //

        // Each bit represents subpass dependency of some kind
        BitMask externalColorDependencies = 0; // If subpass uses color or resolve from external subpass
        BitMask externalDepthDependencies = 0; // If subpass uses depth from external subpass
        BitMask externalInputDependencies = 0; // If subpass uses input from external subpass

        BitMask selfColorDependencies = 0; // If subpass has same attachment as input and color attachment
        BitMask selfDepthDependencies = 0; // If subpass has same attachment as input and depth attachment

        BitMask inputReadDependencies = 0; // If subpass uses attachment as input
        BitMask colorReadWriteDependencies = 0; // If subpass uses attachment as color or resolve
        BitMask depthStencilWriteDependencies = 0; // If subpass uses attachment as depth read
        BitMask depthStencilReadDependencies = 0; // If subpass uses attachment as depth write

        for (al_iterator(attachmentIt, renderPassAttachmentDescriptions))
        {
            VkAttachmentDescription* attachment = get(attachmentIt);
            bool isUsed = false;
            const VkImageLayout initialLayout = attachment->initialLayout;
            VkImageLayout currentLayout = attachment->initialLayout;
            for (al_iterator(subpassIt, subpassIntermediateDatas))
            {
                const BitMask subpassBitMask = BitMask(1) << to_index(subpassIt);
                SubpassIntermediateData* subpass = get(subpassIt);
                VkAttachmentReference* color    = vulkan_find_attachment_reference(subpass->colorAttachmentRefs, to_index(attachmentIt));
                VkAttachmentReference* input    = vulkan_find_attachment_reference(subpass->inputAttachmentRefs, to_index(attachmentIt));
                VkAttachmentReference* resolve  = vulkan_find_attachment_reference(subpass->resolveAttachmentRefs, to_index(attachmentIt));
                VkAttachmentReference* depth    =   subpass->depthStencilReference.attachment != VK_ATTACHMENT_UNUSED && 
                                                    subpass->depthStencilReference.attachment == to_index(attachmentIt) ?
                                                    &subpass->depthStencilReference : nullptr;
                if (!color && !input && !resolve && !depth)
                {
                    continue;
                }
                if (color) al_vk_assert(!depth);
                if (depth) al_vk_assert(!color);
                if (color)
                {
                    if (currentLayout != VK_IMAGE_LAYOUT_GENERAL) currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    if (!isUsed && currentLayout != initialLayout)
                    {
                        externalColorDependencies |= subpassBitMask;
                    }
                    colorReadWriteDependencies |= subpassBitMask;
                }
                if (resolve)
                {
                    if (currentLayout != VK_IMAGE_LAYOUT_GENERAL) currentLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    if (!isUsed && currentLayout != initialLayout)
                    {
                        externalColorDependencies |= subpassBitMask;
                    }
                    colorReadWriteDependencies |= subpassBitMask;
                }
                if (depth)
                {
                    RenderPassCreateInfo::DepthOp depthOp = createInfo->subpasses[to_index(subpassIt)].depthOp;
                    al_vk_assert(depthOp != RenderPassCreateInfo::DepthOp::NOTHING);
                    if (depthOp == RenderPassCreateInfo::DepthOp::READ_WRITE)
                    {
                        if (currentLayout != VK_IMAGE_LAYOUT_GENERAL)
                        {
                            currentLayout = isStencilSupported
                                ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                                : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                        }
                        depthStencilWriteDependencies |= subpassBitMask;
                    }
                    else if (depthOp == RenderPassCreateInfo::DepthOp::READ)
                    {
                        if (currentLayout != VK_IMAGE_LAYOUT_GENERAL)
                        {
                            currentLayout = isStencilSupported
                                ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                : VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
                        }
                    }
                    depthStencilReadDependencies |= subpassBitMask;
                }
                if (input)
                {
                    if (color || resolve || depth)
                    {
                        currentLayout = VK_IMAGE_LAYOUT_GENERAL;
                        if (color) selfColorDependencies |= subpassBitMask;
                        if (depth) selfDepthDependencies |= subpassBitMask;
                    }
                    else if (currentLayout != VK_IMAGE_LAYOUT_GENERAL)
                    {
                        currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                    if (!isUsed && currentLayout != initialLayout)
                    {
                        externalInputDependencies |= subpassBitMask;
                    }
                    inputReadDependencies |= subpassBitMask;
                }

                if (color) color->layout = currentLayout;
                if (input) input->layout = currentLayout;
                if (resolve) resolve->layout = currentLayout;
                if (depth) depth->layout = currentLayout;

                isUsed = true;
            }
            attachment->finalLayout = currentLayout;
        }

        const BitMask allExternalDependencies = externalColorDependencies | externalInputDependencies | externalDepthDependencies;
        const BitMask allSelfDependencies = selfColorDependencies | selfDepthDependencies;
        const uSize numberOfSubpassDependencies = countBits(allExternalDependencies) + countBits(allSelfDependencies) + subpassIntermediateDatas.size - 1;

        Array<VkSubpassDependency> subpassDependencies;
        if (numberOfSubpassDependencies) array_construct(&subpassDependencies, &device->memoryManager.cpu_frameAllocator, numberOfSubpassDependencies);
        defer(if (numberOfSubpassDependencies) array_destruct(&subpassDependencies));
        uSize subpassDependencyIt = 0;

        al_for_each_set_bit(allExternalDependencies, it)
        {
            VkSubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = it;
            if (al_is_bit_set(externalColorDependencies, it))
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependency.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            if (al_is_bit_set(externalInputDependencies, it))
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependency.dstStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT /*| VK_PIPELINE_STAGE_VERTEX_SHADER_BIT*/;
                dependency.dstAccessMask |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            }
            if (al_is_bit_set(externalDepthDependencies, it))
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            }
            subpassDependencies[subpassDependencyIt++] = dependency;
        }

        al_for_each_set_bit(allSelfDependencies, it)
        {
            VkSubpassDependency dependency = {};
            dependency.srcSubpass = it;
            dependency.dstSubpass = it;
            dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            if (al_is_bit_set(selfColorDependencies, it))
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            if (al_is_bit_set(selfDepthDependencies, it))
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }
            dependency.dstStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependency.dstAccessMask |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            subpassDependencies[subpassDependencyIt++] = dependency;
        }

        for (u32 it = 1; it < subpassIntermediateDatas.size; it++)
        {
            // Each subpass must (?) write at least to color or depth attachment
            al_vk_assert(al_is_bit_set(colorReadWriteDependencies, it) || al_is_bit_set(depthStencilWriteDependencies, it));
            al_vk_assert(al_is_bit_set(colorReadWriteDependencies, it - 1) || al_is_bit_set(depthStencilWriteDependencies, it - 1));
            VkSubpassDependency dependency = {};
            dependency.srcSubpass = it - 1;
            dependency.dstSubpass = it;
            dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            if (al_is_bit_set(colorReadWriteDependencies, it - 1))
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            if (al_is_bit_set(depthStencilWriteDependencies, it - 1))
            {
                dependency.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }
            if (al_is_bit_set(inputReadDependencies, it))
            {
                dependency.dstStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT /*| VK_PIPELINE_STAGE_VERTEX_SHADER_BIT*/;
                dependency.dstAccessMask |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            }
            if (al_is_bit_set(colorReadWriteDependencies, it))
            {
                dependency.dstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.dstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }
            if (al_is_bit_set(depthStencilWriteDependencies, it))
            {
                dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;;
            }
            if (al_is_bit_set(depthStencilReadDependencies, it))
            {
                dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            }
            dependency.dstStageMask |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dependency.dstAccessMask |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            subpassDependencies[subpassDependencyIt++] = dependency;
        }

        VkRenderPassCreateInfo renderPassInfo
        {
            .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .attachmentCount    = u32(renderPassAttachmentDescriptions.size),
            .pAttachments       = renderPassAttachmentDescriptions.memory,
            .subpassCount       = u32(subpassDescriptions.size),
            .pSubpasses         = subpassDescriptions.memory,
            .dependencyCount    = numberOfSubpassDependencies ? u32(subpassDependencies.size) : 0,
            .pDependencies      = numberOfSubpassDependencies ? subpassDependencies.memory : nullptr,
        };
        al_vk_check(vkCreateRenderPass(device->gpu.logicalHandle, &renderPassInfo, &device->memoryManager.cpu_allocationCallbacks, &pass->renderPass));
        pass->device = device;
        return pass;

        #undef al_for_each_bit
        #undef al_for_each_set_bit
        #undef al_is_bit_set
    }

    void vulkan_render_pass_destroy(RenderPass* _pass)
    {
        RenderPassVulkan* pass = (RenderPassVulkan*)_pass;
        vkDestroyRenderPass(pass->device->gpu.logicalHandle, pass->renderPass, &pass->device->memoryManager.cpu_allocationCallbacks);
    }
}
