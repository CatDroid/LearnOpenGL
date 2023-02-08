#version 330 core
//layout (points) in;
layout (triangles) in;// 如果跟glDraw*不一样的话 linkprogram没有问题 但使用没有效果
//layout (line_loop, max_vertices = 3) out;    
layout(line_strip, max_vertices = 4) out;  // 输出只能 points  line_strip  triangle_strip
 

in VS_OUT 
{
    vec3 fColor; // 自定义的属性, 不是必须要用接口块来向几何着色器传递数据
} gs_in[];  

// 这个是数组,因为gs传入的是图元 图元可能有多个顶点
// 几何着色器的输入是一个图元的所有顶点
// 因为几何着色器是作用于输入的一组顶点的，从顶点着色器发来输入数据总是会以数组的形式表示出来

/*

内建变量 gl_in 

in gl_Vertex
{
    vec4 gl_Position;   // 这些不用声明 已经包含在内建变量的结构体中
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];

*/

out MY_COLOR // out后的名字必须跟fs的一样 
{
    vec3 fColor; // 如果要透传 可能要更多的??  否则ps也要换,比如pbr很多参数都没有了?? 
} gs_out;        // 如果program要重新编译的话, 为什么不直接换个简单的ps? 


void main() 
{  
#if  0
    gs_out.fColor   = gs_in[0].fColor;
    gl_Position = gl_in[0].gl_Position ;
    EmitVertex();   

    gs_out.fColor   = gs_in[1].fColor;
    gl_Position = gl_in[1].gl_Position ;
    EmitVertex();  

    gs_out.fColor   = gs_in[2].fColor;
    gl_Position = gl_in[2].gl_Position ;
    EmitVertex();  

    gs_out.fColor   = gs_in[0].fColor;
    gl_Position = gl_in[0].gl_Position ;
    EmitVertex(); 

    EndPrimitive(); // 一个图元完成 

#else
   

    for(int i = 0 ; i < 4; i++) // 最后一个顶点跟第一个顶点同一个位置
    {
        gs_out.fColor   = gs_in[i%3].fColor;
        gl_Position = gl_in[i%3].gl_Position ;
        EmitVertex();   
    }
    EndPrimitive();


#endif 
 
}