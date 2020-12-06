#version 330 core

uniform mat4 al_modelMatrix;
uniform mat4 al_vpMatrix;

layout(location = 0) in vec3 a_Position;
//layout(location = 1) in vec3 a_Normal;

out vec4 v_Color;

void main()
{
    vec3 light = normalize(vec3(1, 1, 1));
    v_Color = vec4(0.2, 0.2, 0.8, 1.0);// * dot(normalize(a_Normal), light);
    gl_Position = al_vpMatrix * al_modelMatrix * vec4(a_Position, 1.0);
}
