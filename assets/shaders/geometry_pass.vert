#version 450

struct al_InputGeometryVertex
{
    vec3 positionLS;
    vec3 normalLS;
    vec2 uv;
};

struct al_InputInstanceData
{
    mat4x4 transformWS;
};

layout(set = 0, binding = 0) uniform al_ViewProjection
{
    mat4x4 transformWS;
} al_viewProjection;

layout(set = 1, binding = 0) readonly buffer al_InputGeometry
{
    al_InputGeometryVertex vertices[10];
} al_inputGeometry;

layout(set = 1, binding = 1) readonly buffer al_InputInstances
{
    al_InputInstanceData instanceData[];
} al_inputInstances;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec2 out_uv;

void main() {
    mat4x4 transformWS = al_inputInstances.instanceData[gl_InstanceIndex].transformWS;
    al_InputGeometryVertex vertex = al_inputGeometry.vertices[gl_VertexIndex];
    gl_Position = al_viewProjection.transformWS * transformWS * vec4(vertex.positionLS, 1.0);
    out_position = (transformWS * vec4(vertex.positionLS, 1.0)).xyz;
    out_normal = (transformWS * vec4(vertex.normalLS, 0.0)).xyz;
    out_uv = vertex.uv;
}
