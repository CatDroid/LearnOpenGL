#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec3 aColor;

out VS_OUT {
    vec3 color;
} vs_out;

//out vec3 fColor; // 不使用gs的时候 直接传给ps

void main()
{
	
    vs_out.color = aColor;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
	
	//gl_PointSize = 20.0;  // 必须先 glEnable(GL_PROGRAM_POINT_SIZE);

	//fColor = aColor;
}