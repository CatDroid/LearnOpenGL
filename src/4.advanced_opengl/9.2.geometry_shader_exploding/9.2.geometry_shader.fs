#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

//in vec2 directVsToFs ; 
// The fragment shader uses varying directVsToFs, but previous shader does not write to it.
// 不能跳过gs 从vs直接个ps传递数据

uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = texture(texture_diffuse1, TexCoords);
	 //FragColor = texture(texture_diffuse1, directVsToFs);
}

