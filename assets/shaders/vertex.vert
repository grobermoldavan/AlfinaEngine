#version 330 core

uniform mat4 al_modelMatrix;
uniform mat4 al_vpMatrix;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_Uv;

out vec2 v_Uv;

void main()
{
    v_Uv = a_Uv;
    gl_Position = al_vpMatrix * al_modelMatrix * vec4(a_Position, 1.0);
}
