#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 position;
layout(location = 1) in flat double test;
layout(location = 2) in flat ivec2 uv;
layout(location = 3) in flat int normal;
layout(location = 4) in flat dmat2x3 trf;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform TestPushConstant
{
    vec3 color;
    vec2 color123;
} testPushConstant;

layout(binding = 0) uniform TestUniformBuffer
{
    vec3 uboColor;
};

void main() {
    
}