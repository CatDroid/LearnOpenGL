#version 330 core
out vec4 FragColor;

float near = 0.1; 
float far = 100.0; 
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));
    // 透视投影的逆  转换为相机空间的Near到Far  Near/Far接近黑色  Far/Far=1远的是白色
    // 注意，这里有做取反了  Zn = (f+n)/(f-n) + 2fn/(f-n) * 1/Ze    Ze = [2fn/(f-n) ]/ [Zn - (f+n)/(f-n)] = 2fn / [Zn(f-n)- (f+n)] 因为Ze是从-Near到-Far 这里取反
}

void main()
{             
    float depth = LinearizeDepth(gl_FragCoord.z) / far; // divide by far to get depth in range [0,1] for visualization purposes 为了可视效果 除以far
    FragColor = vec4(vec3(depth), 1.0);
    
    
    //FragColor = vec4(gl_FragCoord.zzz, 1.0);//  屏幕空间的非线性的深度值，物体稍微远一点就变成很白色了，
}
