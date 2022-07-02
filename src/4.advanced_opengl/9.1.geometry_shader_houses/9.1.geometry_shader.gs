#version 330 core
layout (points) in;
//ayout (triangles) in;// �����glDraw*��һ���Ļ� linkprogramû������ ��ʹ��û��Ч��
layout (triangle_strip, max_vertices = 8) out;    // 5+3  ��������ͼԪ, ��8������
//layout (triangle_strip, max_vertices = 7) out;  // ������Ŀ�Ķ��㷢�䲻��ȥ, ���ɲ���һ��ͼƬ�ͻ᲻��

in VS_OUT 
{
    vec3 color;
} gs_in[];     // !!! ��������� ��Ϊgs�������ͼԪ ͼԪ�����ж������
                    // Ҳ���� in vec3 vColor[]; --- ���Ǳ���Ҫ�ýӿڿ����򼸺���ɫ����������

/*

in gl_Vertex	//  �ӿڿ飨Interface Block
{
    vec4  gl_Position;
    float gl_PointSize;
    float gl_ClipDistance[];
} gl_in[];		// Ϊһ�����飬��Ϊ���������ȾͼԪ��������1���Ķ��㣬
                    // ��������ɫ����������һ��ͼԪ�����ж��㡣

// ��Ϊ������ɫ���������������һ�鶥��ģ��Ӷ�����ɫ�����������������ǻ����������ʽ��ʾ����

*/
out GS_OUT 
{
    vec3 fColor;
} gs_out; // gs��ÿ��������������, ���Ҳ����ʹ�ýӿڿ�


void build_house(vec4 position)
{    
    //postion = gl_in[0].gl_Position;
    gs_out.fColor   = gs_in[0].color; // gs_in[0] since there's only one input vertex color��vs�ӿڿ��еı���

    gl_Position = position + vec4(-0.2, -0.2, 0.0, 0.0); // 1:bottom-left   
    EmitVertex();  // !!! ������һ�������ʱ��ÿ�����㽫��ʹ�������fColor�д����ֵ��������Ƭ����ɫ��������
   
    gl_Position = position + vec4( 0.2, -0.2, 0.0, 0.0); // 2:bottom-right
    EmitVertex();
    
	gl_Position = position + vec4(-0.2,  0.2, 0.0, 0.0); // 3:top-left
    EmitVertex();

    gl_Position = position + vec4( 0.2,  0.2, 0.0, 0.0); // 4:top-right
    EmitVertex();  
   
    gl_Position = position + vec4( 0.0,  0.4, 0.0, 0.0); // 5:top
    gs_out.fColor = vec3(1.0, 1.0, 1.0);
    EmitVertex();	// ���䶥��   // !!!  EmitVertex���gs�����(out)��Ϊ��������� ������Ⱦ����(��ֵ��ps)

	EndPrimitive(); // �����5������ϲ���Ϊһ��ͼԪ


	// ÿ�����ǵ���EmitVertexʱ��gl_Position�е������ᱻ��ӵ�ͼԪ������
	// ��EndPrimitive������ʱ�����з������(Emitted)���㶼��"�ϳ�"Ϊָ����"�����ȾͼԪ"

	// ��һ������EmitVertex����֮���ظ�����EndPrimitive�ܹ����ɶ��ͼԪ

    // �ڶ��� triangle_strip ͼԪ
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