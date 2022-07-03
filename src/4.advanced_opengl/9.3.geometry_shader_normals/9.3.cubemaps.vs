#version 330 core
layout (location = 0) in vec3 aPos;
//layout (location = 1) in vec3 aNormal; // VAO最好跟布局修饰符location结合使用
layout (location = 2) in vec2 aTexCoords;


// https://stackoverflow.com/questions/47569224/does-it-matter-if-there-are-gaps-between-uniform-locations-in-opengl-glsl-shader
// Attributes, Uniforms的位置(location)可能是有限制的,但允许跳过部分的
// Uniforms 可能只有16个


out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aTexCoords ;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
