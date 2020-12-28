#version 330 core

uniform sampler2D u_Texture;

layout(location = 0) out vec4 color;

in vec4 v_Color;
in vec2 v_Uv;

float zNear = 0.1; 
float zFar  = 100.0;

float linear_depth(float depth) 
{
    float z = depth * 2.0 - 1.0; 
    return (2.0 * zNear * zFar) / (zFar + zNear - z * (zFar - zNear));	
}

void main()
{
    // Depth
    // float depth = linear_depth(gl_FragCoord.z);
    // color = vec4(vec3(depth), 1.0);

    // Color
    color = v_Color * texture(u_Texture, v_Uv);
}
