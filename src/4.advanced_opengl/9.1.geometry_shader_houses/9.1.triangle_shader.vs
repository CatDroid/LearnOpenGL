#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out MY_COLOR 
{
    vec3 fColor;
} vs_out;

void main()
{
    vs_out.fColor = aColor;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}