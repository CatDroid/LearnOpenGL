#version 330 core
layout (triangles) in; // 输入一个三角形 
layout (triangle_strip, max_vertices=18) out;

uniform mat4 shadowMatrices[6];

//  每个顶点包含属性 gl_Layer gl_Position 和 FragPos

out vec4 FragPos; // FragPos from GS (output per emitvertex) 

void main()
{

    for(int face = 0; face < 6; ++face)
    {
		// !!最重要的!!  内建变量gl_Layer，它指定 "发散出基本图形" 送到 "立方体贴图的哪个面"
		//                  只有当我们有了一个附加到 激活的帧缓冲 的 立方体贴图纹理才有效
		//                  当不管它时，几何着色器就会像往常一样, 把它的基本图形发送到输送管道的下一阶段
		//                  整数
		//	指定输出面
        gl_Layer = face;				// built-in variable that specifies to which face we render.
        
		// 物体的每个图元是个三角形, 每次gs传入三个顶点, 然后生成3*6个顶点 分别在6个点光源方向上
		for(int i = 0; i < 3; ++i) // for each triangle's vertices 
        {

            FragPos = gl_in[i].gl_Position; // 世界坐标系 

            gl_Position = shadowMatrices[face] * FragPos; // 裁剪空间 

            EmitVertex(); // gl_Position gl_Layer FragPos 都可以发射出去, 作为新生成这个顶点的属性 
        
		}    

        EndPrimitive();

    }

} 