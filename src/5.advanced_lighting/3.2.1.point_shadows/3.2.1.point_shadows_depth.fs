#version 330 core
in vec4 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPos); // 取距离  球型 ?? 
    
    // map to [0;1] range by dividing by far_plane 不是near_plane-farplane?
	 // 映射到0到1的范围，把它写入为fragment的深度值
    lightDistance = lightDistance / far_plane;
    
    // write this as modified depth 修改了深度 ? 不使用 gs到ps的  而是改为线性的??
	// 之前使用的是一个空的像素着色器，让OpenGL配置深度贴图的深度值
	// fragment位置和光源位置之间的线性距离
    gl_FragDepth = lightDistance; // gl_FragCoord.z gl_FragDepth都要求是屏幕空间 0~1 
}