#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;

void main()
{
	// 有gs的情况下, 一般vs只要转换到世界坐标系或者摄像机空间
	// 这里 
	// vs转换到世界  
	// gs根据VP, 不同的V, 转换到不同方向的光源空间
    gl_Position = model * vec4(aPos, 1.0);
}