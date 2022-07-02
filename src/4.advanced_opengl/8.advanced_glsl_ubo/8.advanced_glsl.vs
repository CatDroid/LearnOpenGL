#version 330 core
layout (location = 0) in vec3 aPos;

layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

// ��׼ƫ���� �� ����ƫ����(����ڿ鿪ͷ��ƫ��,����һ���ǻ�׼ƫ����������) 

// std140 ���չ̶����� 
// ע�� "����" �����Ǳ���������, ÿ��Ԫ�ص�   ��׼������, ����4N(vec4,16���ֽ�) , �ȽϺ��ڴ�
//        "����"                           ÿ���������Ļ�׼������, ����4N(vec4,16���ֽ�)
// shared ��ͬ��program��uniform����һ��������, ���ǰ���Ӳ�����Ż���ʽ
// packed ��ͬ��program��uniform����һ��, shared/packed��Ҫ��glGetUniformIndices��ѯ

uniform mat4 model;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}  