#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out; // 输出一共6个顶点 共3个图元, 每个图元有两个顶点，组成一个Line

// OpenGL 3.2 Core profile 支持 点、线带和三角形带 points line_strip triangle_strip
//                         但不支持将三角形(triangles)作为输出 

 

in VS_OUT 
{
    vec4 normalClip;
    vec4 positionClip;
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
    // 
    // 并且 后面有几何着色器的顶点着色器甚至不需要写入 gl_Position

	// 在 view space 做 法线绘制  黄色

    gl_Position = projection * gl_in[index].gl_Position;
	gs_out.fColor = vec3(1.0, 1.0, 0.0);
    EmitVertex();

    gl_Position =  (projection *gl_in[index].gl_Position + (projection * vec4(gs_in[index].normal, 0.0)) * MAGNITUDE);
	gs_out.fColor = vec3(1.0, 1.0, 0.0);
    EmitVertex(); // 发射顶点也是设置 gl_Position 这个vs的输出 (几何着色器相当于多个顶点着色器)

    EndPrimitive();
    
    // 在 clip space 做 法线绘制 (后面输入全部用 gs_in) 紫红色
    
    gl_Position = gs_in[index].positionClip;
    gs_out.fColor = vec3(1.0, 0.0, 1.0);
    EmitVertex();
    
    gl_Position = gs_in[index].positionClip + gs_in[index].normalClip * MAGNITUDE;
    gs_out.fColor = vec3(1.0, 0.0, 1.0);
    EmitVertex();
    
    EndPrimitive();
}

void main()
{
    GenerateLine(0); // first vertex normal
    GenerateLine(1); // second vertex normal
    GenerateLine(2); // third vertex normal
}
