#version 330 core

uniform vec4 tint;

layout(location = 0) out vec4 color;

in vec4 v_Color;

void main()
{
    color = v_Color * tint;
}
