#version 330 core

uniform mat4 al_modelMatrix;
uniform mat4 al_vpMatrix;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_Uv;

out vec4 v_Color;
out vec2 v_Uv;

void main()
{
    vec3 light = normalize(vec3(1, 1, 1));
    float dotProduct = max(dot(normalize(a_Normal), light), 0.4);
    v_Color = vec4(dotProduct, dotProduct, dotProduct, 1.0);
    v_Uv = a_Uv;
    gl_Position = al_vpMatrix * al_modelMatrix * vec4(a_Position, 1.0);
}