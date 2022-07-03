#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal; // VAO最好跟布局修饰符location结合使用
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords ;
    TexCoords.x = TexCoords.x + aNormal.x * 0.0000001;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
