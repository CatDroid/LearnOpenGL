#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

//in vec2 directVsToFs ; 
// The fragment shader uses varying directVsToFs, but previous shader does not write to it.
// ��������gs ��vsֱ�Ӹ�ps��������

uniform sampler2D texture_diffuse1;

void main()
{
    FragColor = texture(texture_diffuse1, TexCoords);
	 //FragColor = texture(texture_diffuse1, directVsToFs);
}

