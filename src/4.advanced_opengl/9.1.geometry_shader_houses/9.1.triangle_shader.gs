#version 330 core
//layout (points) in;
layout (triangles) in;// �����glDraw*��һ���Ļ� linkprogramû������ ��ʹ��û��Ч��
//layout (line_loop, max_vertices = 3) out;    
layout(line_strip, max_vertices = 4) out;  // ���ֻ�� points  line_strip  triangle_strip
 

in VS_OUT 
{
    vec3 fColor; // �Զ��������, ���Ǳ���Ҫ�ýӿڿ����򼸺���ɫ����������
} gs_in[];  

// ���������,��Ϊgs�������ͼԪ ͼԪ�����ж������
// ������ɫ����������һ��ͼԪ�����ж���
// ��Ϊ������ɫ���������������һ�鶥��ģ��Ӷ�����ɫ�����������������ǻ����������ʽ��ʾ����

/*

�ڽ����� gl_in 

in gl_Vertex
{
    vec4 gl_Position;   // ��Щ�������� �Ѿ��������ڽ������Ľṹ����
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];

*/

out MY_COLOR // out������ֱ����fs��һ�� 
{
    vec3 fColor; // ���Ҫ͸�� ����Ҫ�����??  ����psҲҪ��,����pbr�ܶ������û����?? 
} gs_out;        // ���programҪ���±���Ļ�, Ϊʲô��ֱ�ӻ����򵥵�ps? 


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

    EndPrimitive(); // һ��ͼԪ��� 

#else
   

    for(int i = 0 ; i < 4; i++) // ���һ���������һ������ͬһ��λ��
    {
        gs_out.fColor   = gs_in[i%3].fColor;
        gl_Position = gl_in[i%3].gl_Position ;
        EmitVertex();   
    }
    EndPrimitive();


#endif 
 
}