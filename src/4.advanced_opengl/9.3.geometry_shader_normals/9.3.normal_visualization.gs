#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out; // ���һ��6������ ��3��ͼԪ, ÿ��ͼԪ���������㣬���һ��Line

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
	// gl_in[]. gl_Position��vs�����,  vs�����������view space�Ķ�������ͷ�������

	// ��viewspace������͹��

    gl_Position = projection * gl_in[index].gl_Position;
	gs_out.fColor = vec3(1.0, 1.0, 0.0);
    EmitVertex();

    gl_Position = projection * (gl_in[index].gl_Position + vec4(gs_in[index].normal, 0.0) * MAGNITUDE);
	gs_out.fColor = vec3(1.0, 0.0, 0.0); 
    EmitVertex(); // ���䶥��Ҳ������ gl_Position ���vs����� (������ɫ���൱�ڶ��������ɫ��)

    EndPrimitive();
}

void main()
{
    GenerateLine(0); // first vertex normal
    GenerateLine(1); // second vertex normal
    GenerateLine(2); // third vertex normal
}