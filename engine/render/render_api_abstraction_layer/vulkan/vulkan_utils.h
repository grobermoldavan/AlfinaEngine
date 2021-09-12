#ifndef AL_VULKAN_UTILS_H
#define AL_VULKAN_UTILS_H

#include "vulkan_base.h"
#include "../base/enums.h"
#include "../base/spirv_reflection/spirv_reflection.h"

namespace al::utils
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR    capabilities;
        Array<VkSurfaceFormatKHR>   formats;
        Array<VkPresentModeKHR>     presentModes;
    };

    struct ViewportScissor
    {
        VkViewport viewport;
        VkRect2D scissor;
    };

    PointerWithSize<const char*> get_required_validation_layers();
    PointerWithSize<const char*> get_required_instance_extensions();
    PointerWithSize<const char*> get_required_device_extensions();

    Array<VkLayerProperties> get_available_validation_layers(AllocatorBindings* bindings);
    Array<VkExtensionProperties> get_available_instance_extensions(AllocatorBindings* bindings);

    VkDebugUtilsMessengerCreateInfoEXT get_debug_messenger_create_info(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* userData = nullptr);
    VkDebugUtilsMessengerEXT create_debug_messenger(VkDebugUtilsMessengerCreateInfoEXT* createInfo, VkInstance instance, VkAllocationCallbacks* callbacks = nullptr);
    void destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks* callbacks = nullptr);

    VkCommandPool create_command_pool(VkDevice device, u32 queueFamilyIndex, VkAllocationCallbacks* callbacks, VkCommandPoolCreateFlags flags);
    void destroy_command_pool(VkCommandPool pool, VkDevice device, VkAllocationCallbacks* callbacks);

    SwapChainSupportDetails create_swap_chain_support_details(VkSurfaceKHR surface, VkPhysicalDevice device, AllocatorBindings* bindings);
    void destroy_swap_chain_support_details(SwapChainSupportDetails* details);

    VkSurfaceFormatKHR choose_swap_chain_surface_format(Array<VkSurfaceFormatKHR> available);
    VkPresentModeKHR choose_swap_chain_surface_present_mode(Array<VkPresentModeKHR> available);
    VkExtent2D choose_swap_chain_extent(u32 windowWidth, u32 windowHeight, VkSurfaceCapabilitiesKHR* capabilities);

    Tuple<bool, u32> pick_graphics_queue(Array<VkQueueFamilyProperties> familyProperties);
    Tuple<bool, u32> pick_present_queue(Array<VkQueueFamilyProperties> familyProperties, VkPhysicalDevice device, VkSurfaceKHR surface);
    Tuple<bool, u32> pick_transfer_queue(Array<VkQueueFamilyProperties> familyProperties);

    template<uSize Size>
    Array<VkDeviceQueueCreateInfo> get_queue_create_infos(Tuple<bool, u32> (&queues)[Size], AllocatorBindings* bindings);

    VkCommandPoolCreateInfo command_pool_create_info(u32 queueFamilyIndex, VkCommandPoolCreateFlags flags);

    bool pick_depth_stencil_format(VkPhysicalDevice physicalDevice, VkFormat* result);
    Array<VkPhysicalDevice> get_available_physical_devices(VkInstance instance, AllocatorBindings* bindings);
    Array<VkQueueFamilyProperties> get_physical_device_queue_family_properties(VkPhysicalDevice physicalDevice, AllocatorBindings* bindings);
    bool does_physical_device_supports_required_extensions(VkPhysicalDevice device, PointerWithSize<const char* const> extensions, AllocatorBindings* bindings);
    bool does_physical_device_supports_required_features(VkPhysicalDevice device, VkPhysicalDeviceFeatures* requiredFeatures);

    VkImageType pick_image_type(VkExtent3D imageExtent);

    VkCommandBuffer create_command_buffer(VkDevice device, VkCommandPool pool, VkCommandBufferLevel level);

    VkShaderModule create_shader_module(VkDevice device, Tuple<u32*, uSize> bytecode, VkAllocationCallbacks* allocationCb = nullptr);
    void destroy_shader_module(VkDevice device, VkShaderModule module, VkAllocationCallbacks* allocationCb = nullptr);

    bool get_memory_type_index(VkPhysicalDeviceMemoryProperties* props, u32 typeBits, VkMemoryPropertyFlags properties, u32* result);

    TextureFormat to_texture_format(VkFormat vkFormat);
    VkFormat to_vk_format(TextureFormat format);
    VkAttachmentLoadOp to_vk_load_op(AttachmentLoadOp loadOp);
    VkAttachmentStoreOp to_vk_store_op(AttachmentStoreOp storeOp);
    VkPolygonMode to_vk_polygon_mode(PipelinePoligonMode mode);
    VkCullModeFlags to_vk_cull_mode(PipelineCullMode mode);
    VkFrontFace to_vk_front_face(PipelineFrontFace frontFace);
    VkSampleCountFlagBits to_vk_sample_count(MultisamplingType multisample);
    VkSampleCountFlagBits pick_sample_count(VkSampleCountFlags desired, VkSampleCountFlags supported);
    VkStencilOp to_vk_stencil_op(StencilOp op);
    VkCompareOp to_vk_compare_op(CompareOp op);
    VkBool32 to_vk_bool(bool value);
    VkDescriptorType to_vk_descriptor_type(SpirvReflection::Uniform::Type type);
    VkShaderStageFlags to_vk_stage_flags(ProgramStageFlags stages);

    VkPipelineShaderStageCreateInfo shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule module, const char* pName);
    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info(u32 bindingsCount = 0, const VkVertexInputBindingDescription* bindingDescs = nullptr, u32 attrCount = 0, const VkVertexInputAttributeDescription* attrDescs = nullptr);
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info(VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable = VK_FALSE);
    ViewportScissor default_viewport_scissor(u32 width, u32 height);
    VkPipelineViewportStateCreateInfo viewport_state_create_info(VkViewport* viewport, VkRect2D* scissor);
    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
    VkPipelineMultisampleStateCreateInfo multisample_state_create_info(VkSampleCountFlagBits resterizationSamples = VK_SAMPLE_COUNT_1_BIT);
    VkPipelineColorBlendStateCreateInfo color_blending_create_info(PointerWithSize<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates);
    VkPipelineDynamicStateCreateInfo dynamic_state_default_create_info();

    VkAccessFlags image_layout_to_access_flags(VkImageLayout layout);
    VkPipelineStageFlags image_layout_to_pipeline_stage_flags(VkImageLayout layout);
}

#endif
