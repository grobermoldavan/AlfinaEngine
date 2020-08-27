#version 330 core

uniform mat4 al_modelMatrix;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;

out vec4 v_Color;

void main()
{
    v_Color = a_Color;
    gl_Position = al_modelMatrix * vec4(a_Position, 1.0);    
}
