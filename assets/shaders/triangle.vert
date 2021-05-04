#version 450
#extension GL_ARB_separate_shader_objects : enable

//struct Vertex
//{
//    vec3 position;
//    vec3 color;
//};
//
//layout(binding = 0) readonly buffer Vertices
//{
//    Vertex vertices[];
//};
//
//layout(location = 0) out vec3 fragColor;
//
//void main()
//{
//    gl_Position = vec4(vertices[gl_VertexIndex].position, 1.0);
//    fragColor = vertices[gl_VertexIndex].color;
//}

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(push_constant) uniform TestPushConstant
{
    vec3 color;
} testPushConstant;

layout(binding = 0) uniform TestUniformBuffer
{
    vec3 uboColor;
};

layout(location = 0) out vec3 fragColor;

// vec3 colors[3] = vec3[](
//     vec3(1.0, 0.0, 0.0),
//     vec3(0.0, 1.0, 0.0),
//     vec3(0.0, 0.0, 1.0)
// );

void main() {
    gl_Position = vec4(position, 1.0);
    // fragColor = colors[gl_VertexIndex % 3] * position.z;
    if ((gl_VertexIndex % 3) == 0)
        fragColor = testPushConstant.color;
    if ((gl_VertexIndex % 3) == 1)
        fragColor = uboColor;
    if ((gl_VertexIndex % 3) == 2)
        fragColor = vec3(0, 1, 0);
}