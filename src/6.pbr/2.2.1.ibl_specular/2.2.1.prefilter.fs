#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;
uniform float roughness;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits) 
{
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 Hammersley(uint i, uint N)
{
	return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}
// ----------------------------------------------------------------------------
vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
	float a = roughness*roughness;
	
	float phi = 2.0 * PI * Xi.x; // 这个是GGX D法线分布函数 对应的pdf 
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta; 
	// 上面结果是 X-Y-Z 坐标系 下的 高光波瓣， 以Z轴为中心 对称
	
	// 要把上面的Z轴转换为以N方向为中心  现在已经知道了Z轴映射到 N向量(世界坐标系 or 其他)，
	// 所以只要 求得N向量所在坐标系，以N向量垂直(正交)的两个向量即可(用N向量所在坐标系表述)

	// from tangent-space H vector to world-space sample vector
	// 避免N向量刚好是 0.0,0.0,1.0, 这样叉乘为零向量(tangent)
	// 对于一个宏表面, 这个映射的坐标系是一样的
	vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}
// ----------------------------------------------------------------------------
void main()
{		
    vec3 N = normalize(WorldPos);
    
	// 这里把宏表面始终面向视线方向(输出方向 cubemap的方向)
	// 真实的情况 应该是要考虑V和N的方向的, 也就是N一定的情况下 视线在不同的方向, 分割求和LD项也不一样
	// 做这样假设之后 V H(N) 和 L 的方向都会比较近(H方向 大多数在N方向附近, 而现在V对齐N, 所以H方向也大多在V附近)
    // make the simplyfying assumption that V equals R equals the normal 
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;
    
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
        // 生成 低差异序列 代替 均匀分布pdf  
		vec2 Xi = Hammersley(i, SAMPLE_COUNT);
		// 根据 GGX法线分布pdf 生成H向量(这里的N等于V 所以H也集中在V附近)
        vec3 H = ImportanceSampleGGX(Xi, N, roughness); 
		// 计算光线方向 
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

		// 光线 与 宏表面 超过90度, 跳过
        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
			// 根据GGX法线分布函数 得到法线N的宏表面上 H方向的微表面数量
            // sample from the environment's mip level based on roughness/pdf
            float D   = DistributionGGX(N, H, roughness);
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);

			// D * NdotH 是 GGX的pdf(H)的公式(H方向上的立体角), 对应pdf(θ_h, φ_h) =  D * cosθ * sinθ
			// 这里需要从H向量 转换为L向量(光线方向)的pdf  
			// pdf(L) = pdf(h) /  (4.0 * HdotV)   HdotV = HdotL 
			// 参考最后: https://zhuanlan.zhihu.com/p/78146875
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001; 
 
			// !! 预过滤卷积时，不直接采样环境贴图，
			// 而是基于积分的 PDF 和粗糙度采样环境贴图的 mipmap ，以减少伪像(失真)
			// 概率低的采样向量, 代表这个方向的采样比较少, 应该平均更大的区域, 也就要选择mipmap level更大(图更小)

			// envCubemap 的分辨率是512x512  resolution of source cubemap (per face)
            float resolution = 512.0;  
			// (6*4*π) / (size*size) mip0级的环境贴图上, 每分辨率对应的立体角大小 
            float saTexel  = 4.0 * PI / (6.0 * resolution * resolution); 
			// 1 / (N*pdf) 根据pdf计算出的, 每次采样的立体角大小 (数学期望E(∑(saSample))=2π, 刚好是半球的球面度)
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001); 

			// saSample / saTexel 每次采样 对应cubemap上的分辨率 (正方形区域的面积, 需要采样mip0的像素数)
			// sqrt(saSample / saTexel) 也就是 0.5 , 是得到边长分辨率 ，
			// 也就是这次采样"要平均这么多的分辨率"，通过 log2 得到哪一层

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 
            
			// 选择 environmentMap 对应的 mipLevel 
            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;

			// 分离求和的 LD项  公式是 1/∑(n*lk)  * ∑ ( L(l) * (n*lk) )
            totalWeight      += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;

    FragColor = vec4(prefilteredColor, 1.0);
}
