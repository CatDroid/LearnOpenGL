#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out; // 输出一共6个顶点 共3个图元, 每个图元有两个顶点，组成一个Line

in VS_OUT 
{
    vec3 normal;
} gs_in[];

out GS_OUT 
{
	vec3 fColor ;
} gs_out ;

const float MAGNITUDE = 0.2;

uniform mat4 projection;

void GenerateLine(int index)
{
	// gl_in[]. gl_Position是vs的输出,  vs现在输出的是view space的顶点坐标和法线向量

	// 在viewspace做法线凸出

    gl_Position = projection * gl_in[index].gl_Position;
	gs_out.fColor = vec3(1.0, 1.0, 0.0);
    EmitVertex();

    gl_Position = projection * (gl_in[index].gl_Position + vec4(gs_in[index].normal, 0.0) * MAGNITUDE);
	gs_out.fColor = vec3(1.0, 0.0, 0.0); 
    EmitVertex(); // 发射顶点也是设置 gl_Position 这个vs的输出 (几何着色器相当于多个顶点着色器)

    EndPrimitive();
}

void main()
{
    GenerateLine(0); // first vertex normal
    GenerateLine(1); // second vertex normal
    GenerateLine(2); // third vertex normal
}