#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 WorldPos;

// 不移动的背景(相机的平移组件被忽略)
void main()
{
    // 天空盒的原点永远在世界坐标系的原点
    WorldPos = aPos;

    // 天空盒一个特点 不跟着相机位移,但是跟着view旋转缩放
	mat4 rotView = mat4(mat3(view));
	vec4 clipPos = projection * rotView * vec4(WorldPos, 1.0);

    // xyww 技巧，用在天空盒, 它确保渲染的立方体片段的深度值始终以 1.0 结束，即最大深度值(同时需要将深度比较函数更改为 GL_LEQUAL)
    //
	gl_Position = clipPos.xyww;
}
