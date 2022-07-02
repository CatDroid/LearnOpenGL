#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec2 texCoords;
} vs_out;

//out vec2 directVsToFs ; 

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vs_out.texCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0); 

	//directVsToFs = aTexCoords;// ����ֱ��vs��ps
	// ���Ӵ���: The fragment shader uses varying directVsToFs, but previous shader does not write to it.
	// ��������gs ��vsֱ�Ӹ�ps��������
}