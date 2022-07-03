#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec2 texCoords;
    vec4 positionViewSpace;
} vs_out;

//out vec2 directVsToFs ; 

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vs_out.texCoords = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0); // clip space
    
    vs_out.positionViewSpace = view * model * vec4(aPos, 1.0); // 改成view space
    
	//directVsToFs = aTexCoords;// 测试直接vs到ps
	// 链接错误: The fragment shader uses varying directVsToFs, but previous shader does not write to it.
	// 不能跳过gs 从vs直接个ps传递数据
}
