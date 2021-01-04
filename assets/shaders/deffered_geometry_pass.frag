#version 330 core

uniform sampler2D texDiffuse;

in vec3 v_Position;
in vec3 v_Normal;
in vec2 v_Uv;

layout (location = 0) out vec3 g_Position;
layout (location = 1) out vec3 g_Normal;
layout (location = 2) out vec3 g_Albedo;

void main()
{
    g_Position  = v_Position;
    g_Normal    = normalize(v_Normal);
    g_Albedo    = texture(texDiffuse, v_Uv).rgb;
}
