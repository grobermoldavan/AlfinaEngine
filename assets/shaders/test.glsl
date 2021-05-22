#version 450
#extension GL_ARB_separate_shader_objects : enable

struct Test
{
    int a;
    float b;
};

layout(location = 0) in vec3 position;
layout(location = 1) in flat double test;
layout(location = 2) in flat ivec2 uv;
layout(location = 3) in flat int normal;
layout(location = 4) in flat double trf;
layout(location = 5) in flat Test inStruct;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform TestPushConstant
{
    vec3 color;
    vec2 color123;
} testPushConstant222;

layout(binding = 0) uniform TestUniformBuffer
{
    vec3 uboColor;
    int wtf;
};

layout(set = 1, binding = 1) uniform TestUniformBuffer2
{
    float a;
};

layout(set = 2, binding = 1) uniform sampler3D texSampler1;
layout(set = 2, binding = 2) uniform sampler2D texSampler2;
layout(set = 2, binding = 3) uniform sampler3D texSampler3;

layout (input_attachment_index = 1, set = 3, binding = 1) uniform subpassInput inputDepth;

void main() {
    
}