#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

// 改成使用接口块 来传递vs到ps的数据
//out vec2 TexCoords;
//out vec3 Normal;
//out vec3 FragPos;
out VS_OUT // 接口块名字要一样 
{
	vec2 TexCoords;
	vec3 Normal;
	vec3 FragPos;
} vs_out; // 实例名字vs和ps可以不一样 

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vs_out.TexCoords = aTexCoords;
    
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;
    
     vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
