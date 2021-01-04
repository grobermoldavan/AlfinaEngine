#version 330 core

uniform sampler2D texScreen;

layout(location = 0) out vec4 color;
  
in vec2 v_Uv;

void main()
{ 
    color = texture(texScreen, v_Uv);
}
