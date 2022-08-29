#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main()
{		
	// cubemap的世界坐标/模型坐标(向量)作为过原点的切面的法线, 对于世界坐标
    // 给定法线, 计算所有输入的环境光辐射率(radiance)
	// 半球的朝向，就是这个法线 
	// 预积分/预计算 每个可能的半球朝向的辐照度(irradiance), 即涵盖了所有可能的出射方向 wo

	// 后续要获取,  某个片段上间接漫反射光的总辐照度, 只要用表面的法线,采样辐照图立方图, 就可得到
	// 以这个法线对齐的半球的积分的总辐照度 (跟视线方向无关)

    vec3 N = normalize(WorldPos);

    vec3 irradiance = vec3(0.0);   
    
    // tangent space calculation from origin point
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
       
    float sampleDelta = 0.025;
    float nrSamples = 0.0;

	// 对于立方体贴图的每个纹素，在纹素所代表的方向的半球 Ω 内生成固定数量的采样向量，并对采样结果取平均值。
	// 
	// 数量固定的采样向量将均匀地分布在半球内部。
	// 
	// 半球内均匀间隔或随机取方向, 获得一个相当精确的辐照度近似值，从而离散地计算积分
	//
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
			// 球坐标系 转换为 直角坐标系(切向空间)
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

			// 漫反射光/间接光的 辐射率/辐射度 L(p,wi) 从 环境立方体贴图 获取
            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
			// 物理意义:
			//		cos(θ) 较大角度的光较弱
			//		sin(θ) 则用于权衡较高半球区域 的 采样面积小, 贡献度也小

			// 积分改成黎曼和--离散

			// +=  radiance(p, wi) * cos(theta) * sin(theta) * (2π/n1) * ((π/2)/n2)    * ( kd * c / π )

			// 这里最后的结果不包含 kd * c ,  这两个参数跟物体材质相关 

            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples)); // nrSamples = n1*n2;
    
    FragColor = vec4(irradiance, 1.0); 
	// 该方向上的辐照度
	// 反射方程中的 漫发射部分-- 与输出方向(视线方向)无关, 只跟入射方向和表面法线方向有关(跟Phong模型类似)
}
