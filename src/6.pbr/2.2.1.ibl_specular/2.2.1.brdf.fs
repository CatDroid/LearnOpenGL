#version 330 core
out vec2 FragColor;
in vec2 TexCoords;

const float PI = 3.14159265359;
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
	
	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;
	
    // IntegrateBRDF的N 假设就是在z轴方向(0,0,1)
	// from tangent-space H vector to world-space sample vector
	vec3 up          = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent   = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);
	
	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    // Smith几何项, 在直接光光照 和  间接光光照 的k公式是不一样的
	// Kdirect = (α + 1)^2 / 8 
	// Kibl      = (α)^2       / 2 
	// G_smith= G_ggx(n,v,k) * G_ggx (n,l,k)
	// G_ggx   = (n*v) / [(n*v)(1-k) + k]

    // note that we use a different k for IBL
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec2 IntegrateBRDF(float NdotV, float roughness)
{
    // 假设N在z轴,并且V在XOZ平面
    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0; 

    // 假设N在z轴
    vec3 N = vec3(0.0, 0.0, 1.0);
    
    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        // generates a sample vector that's biased towards the
        // preferred alignment direction (importance sampling).
        // 低差异序列 代替均匀分布
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        // 生成符合 GGX法线分布pdf 的H向量
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        // 计算在H向量对应的入射光线方向
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        // 因为N就是z轴, 单位向量与z轴内积结果 等于单位向量的z轴坐标
        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if(NdotL > 0.0)
        {
            float G = GeometrySmith(N, V, L, roughness);
            // G' = G / [ (n*h_k) (n*v)]
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            // 菲涅尔公式 反射占入射光线的比例
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    
    // 分离求和 DFG项目，主要把基础反射率F0, 提取出来(与材质相关)
    // DFG =     F0    *   [ 1/N * ∑(1-Fc)G' ]  +   1/N * ∑(Fc)G'
    //       ---材质相关---  ------- A ---------          ----- B -----
    // G' = G * (v*h) / (n*h)(n*v)
    // h方向 由法线分布函数GGX-NDF生成, 需要roughness和N两个参数
    
    // DFG 虽然要N方向，H方向，但是最终只要 N和V N和H V和H 的夹角关系,
    //     而H是由N生成, 并且在N的周围，最后也只要H_V H_N的夹角, 所以N的具体方向(或者说所在坐标系)没有关系
    //     Smith G几何项 也只跟 N_V N_L 夹角有关系
    //     总结, 只要给出N_V夹角, 假设N=(0,0,1), 和V向量在XOZ平面 ==> V向量 ==> H向量 ==> L向量  ==> 得到N_H V_H, N_V N_L等夹角关系
    //          加上rougness, 就可以得到 A(菲涅尔因子) 和 B(菲涅尔偏移)
    
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    
    return vec2(A, B);
}
// ----------------------------------------------------------------------------
void main() 
{
    // BRDF积分图 二维 参数是纹理坐标, 分别表示 NdotV 夹角, roughness粗糙度
    vec2 integratedBRDF = IntegrateBRDF(TexCoords.x, TexCoords.y);
    FragColor = integratedBRDF;
}
