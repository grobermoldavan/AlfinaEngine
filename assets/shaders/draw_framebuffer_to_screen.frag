#version 330 core

uniform sampler2D texScreenDiffuse;

layout(location = 0) out vec4 color;
  
in vec2 v_Uv;

void main()
{
    color = texture(texScreenDiffuse, v_Uv);
}
