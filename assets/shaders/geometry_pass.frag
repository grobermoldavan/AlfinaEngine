#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec3 out_position;
layout(location = 2) out vec3 out_normal;

layout(binding = 0) uniform sampler2D diffuseTexture;

void main() {
    out_albedo = texture(diffuseTexture, in_uv);
    out_position = in_position;
    out_normal = in_normal;
}
