#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;
out float Ze;
out float Zc;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    
    //if (pos.w > 0)
    //{
    //    pos.w = -pos.w;
    //} // 能够显示的 Wc (裁剪坐标系) 应该是正数 Wc=-Ze, 所以加上这段端之后，即使gl_Position=xyww裁剪后z都是1,但是Wc<0, 所以都会被裁掉
    
    gl_Position = pos.xyww; // xyzw   NDC中的深度 = z/w
    
    // 将输出位置的z分量等于它的w分量，让z分量永远等于1.0，这样子的话，当透视除法执行之后，z分量会变为w / w = 1.0
    
    // 欺骗深度缓冲，让它认为天空盒有着最大的深度值1.0，只要它前面有一个物体，深度测试就会失败
    
    
    //vec4 pos = projection * view * vec4(aPos, 1.0);
    //gl_Position = pos.xyzw;
    
    Ze = (view * vec4(aPos, 1.0)).z;
    Zc = pos.z;  // 正常情况下, 只有 |Zc|<=Wc 才会显示出来  如果Wc<0, 一定不会显示出来, 因为不满足-Wc(正数)<=Xc,Yc,Zc<=Wc(负数), 除非是0否则没有一个数能同时满足两边
}  
