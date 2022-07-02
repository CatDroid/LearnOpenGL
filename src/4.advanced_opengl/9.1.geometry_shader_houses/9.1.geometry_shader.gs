#version 330 core
layout (points) in;
//ayout (triangles) in;// 如果跟glDraw*不一样的话 linkprogram没有问题 但使用没有效果
layout (triangle_strip, max_vertices = 8) out;    // 5+3  生成两个图元, 共8个顶点
//layout (triangle_strip, max_vertices = 7) out;  // 超过数目的顶点发射不出去, 构成不了一个图片就会不画

in VS_OUT 
{
    vec3 color;
} gs_in[];     // !!! 这个是数组 因为gs传入的是图元 图元可能有多个顶点
                    // 也可以 in vec3 vColor[]; --- 不是必须要用接口块来向几何着色器传递数据

/*

in gl_Vertex	//  接口块（Interface Block
{
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];		// 为一个数组，因为大多数的渲染图元包含多于1个的顶点，
                    // 而几何着色器的输入是一个图元的所有顶点。

// 因为几何着色器是作用于输入的一组顶点的，从顶点着色器发来输入数据总是会以数组的形式表示出来

*/
out GS_OUT 
{
    vec3 fColor;
} gs_out; // gs给每个顶点的输出属性, 这个也可以使用接口块


void build_house(vec4 position)
{    
    //postion = gl_in[0].gl_Position;
    gs_out.fColor   = gs_in[0].color; // gs_in[0] since there's only one input vertex color是vs接口块中的变量

    gl_Position = position + vec4(-0.2, -0.2, 0.0, 0.0); // 1:bottom-left   
    EmitVertex();  // !!! 当发射一个顶点的时候，每个顶点将会使用最后在fColor中储存的值，来用于片段着色器的运行
   
    gl_Position = position + vec4( 0.2, -0.2, 0.0, 0.0); // 2:bottom-right
    EmitVertex();
    
	gl_Position = position + vec4(-0.2,  0.2, 0.0, 0.0); // 3:top-left
    EmitVertex();

    gl_Position = position + vec4( 0.2,  0.2, 0.0, 0.0); // 4:top-right
    EmitVertex();  
   
    gl_Position = position + vec4( 0.0,  0.4, 0.0, 0.0); // 5:top
    gs_out.fColor = vec3(1.0, 1.0, 1.0);
    EmitVertex();	// 发射顶点   // !!!  EmitVertex会把gs的输出(out)作为顶点的属性 传到渲染管线(插值给ps)

	EndPrimitive(); // 上面的5个顶点合并成为一个图元


	// 每次我们调用EmitVertex时，gl_Position中的向量会被添加到图元中来。
	// 当EndPrimitive被调用时，所有发射出的(Emitted)顶点都会"合成"为指定的"输出渲染图元"

	// 在一个或多个EmitVertex调用之后重复调用EndPrimitive能够生成多个图元

    // 第二个 triangle_strip 图元
	 gs_out.fColor   = vec3(1.0, 0.5, 0.0);
	gl_Position = position + vec4(-0.1, -0.1, -0.5, 0.0); // 1:bottom-left   
    EmitVertex();   
    gl_Position = position + vec4( 0.1, -0.1, -0.5, 0.0); // 2:bottom-right
    EmitVertex();
	gl_Position = position + vec4(-0.1,  0.1, -0.5, 0.0); // 3:top-left
    EmitVertex();
	EndPrimitive();

}

void main() {    
    build_house(gl_in[0].gl_Position);
}