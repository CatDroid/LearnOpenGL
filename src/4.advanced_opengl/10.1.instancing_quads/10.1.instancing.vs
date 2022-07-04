#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aOffset;

out vec3 fColor;

void main()
{
    // 正方体模型是原点为中心，在-0.05~+0.05方位的
    //aPos = aPos * gl_InstanceID / 100.0;
    // in是一个只读变量 Left-hand-side of assignment must not be read-only
    vec2 pos = aPos * gl_InstanceID / 100.0;
    fColor = aColor;
    gl_Position = vec4(pos + aOffset, 0.0, 1.0);
}
