#version 330 core

uniform mat4 al_modelMatrix;
uniform mat4 al_vpMatrix;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_Uv;

out vec3 v_Position;
out vec3 v_Normal;
out vec2 v_Uv;

void main()
{
    v_Position  = (al_vpMatrix * al_modelMatrix * vec4(a_Position, 1.0)).xyz;
    v_Normal    = (al_modelMatrix * vec4(a_Normal, 0.0)).xyz; //Probably want WS normals here (al_vpMatrix * al_modelMatrix * vec4(a_Normal, 0.0)).xyz;
    v_Uv        = a_Uv;
    gl_Position = al_vpMatrix * al_modelMatrix * vec4(a_Position, 1.0);
}
