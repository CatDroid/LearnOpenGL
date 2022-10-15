#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// material parameters
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

// IBL
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

// lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

uniform vec3 camPos;

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
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

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
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    // 在掠射角 cosTheta = 0, F = max(vec3(1.0 - roughness), F0)  
    //                                  粗糙度比较高情况下, F比较小, 但不会小于F0
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
// ----------------------------------------------------------------------------
void main()
{		
    vec3 N = Normal;
    vec3 V = normalize(camPos - WorldPos);
    vec3 R = reflect(-V, N); 

    // 法向入射反射率: 如果是金属,就会使用albedo作为其基础反射率;电介质则都用F0=0.04
    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

	// 直接光照, 点光源 
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) // 4个点光源
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;       // 没有用kD kQ 二次项 一次项 常数项的 点光源衰减

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);       // NDF 会过滤掉只剩下法线为H的微平面
        float G   = GeometrySmith(N, V, L, roughness);      // Smith几何项 N_dot_V N_dot_L 有关系
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);// F 项计算 用的是 微平面法线和视线的夹角 不是宏表面法线和视线夹角
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero 避免除零
        vec3 specular = numerator / denominator;
        
         // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        // 金属没有漫反射
        kD *= 1.0 - metallic;	                
            
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }   
    
	// 间接光照--- 环境光照 使用IBL得到
    // ambient lighting (we now use IBL as the ambient term)

	// 1. 计算反射率(向量, 菲涅尔公式, 考虑粗糙度) -- 间接光菲涅尔项
	// 
    //    IBL漫反射部分, 使用辐照度图的话，原来是把kD当做常数，没有放到 辐照图立方体贴图 中，
	//    但是实际 kD = 1-Ks = 1-F , F应该是 微表面法线H 和 视线V 的夹角 , 但为了使用 辐照图立方体贴图
	//    这里F直接使用了 宏表面法线N 和 视线V 的夹角 (近似 H和V的夹角)
	//    对于光滑表面 H其实接近N 没有什么影响; 对于粗糙表面， 掠视角会有失真, 比较强边沿光
	// 
	//    Sébastien Lagarde提供了一种补救措施
	//    修改90度观察角处的fresnel，从1改为1-roughness，以此修正roughness比较高时掠视角的artifact
	//    在掠射角 fresnelSchlickRoughness 比 fresnelSchlick 要小 kS会小于后者的, 
	//    因此 后面的 vec3 specular = prefilteredColor * (F * brdf.x + brdf.y) 会小了
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	// 金属度 相当于吸收了折射的光线, kD会减少了, 漫反射(折射中的部分)+镜面反射就不完全等于入射
    
	// 2. 间接光照--漫反射部分,使用 辐照度立方体贴图 kD * c / π * (∫ Li(ωi,p) * n*ωi d ωi  )  
	//                   辐照度立方体贴图 包含公式中的 1/ π * (∫ Li(ωi,p) * n*ωi d ωi  )  
	//                   积分使用了 pdf(θ,φ)=1/(π^2) 平均 重要性采样
	//                   注意 这里的kD当作常数了, 实际应该跟H_dot_V相关 
	//                   辐照度立方体贴图 表示 宏表面在不同方向(宏表面的法线)的 间接光的漫反射 积分值 
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * albedo;
    
	// 3. 间接光照--镜面反射部分,使用分割求和近似法 预滤波积分图 cubemap + BRDF LUT(NdotV, roughness) 
    // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
    const float MAX_REFLECTION_LOD = 4.0;  
	// 预滤波积分图  LOD可以是0到4, 5个mipmap级别
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb; 
	
	// BRDF Lut图  s:宏观表面的法线N 与 视线夹角   t:粗糙度 
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg; 

	 // 菲涅尔因子和菲涅尔偏差 得到 BRDF反射方程 镜面高光的 积分结果
	 // !! 注意这里用的F 是 考虑了粗糙度的菲涅尔 不直接是 垂直入射的反射率F0  
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	 // 得到间接光光照 
    vec3 ambient = (kD * diffuse + specular) * ao;
    
	// 最终颜色 = 直接光光照 + 间接光光照 
    vec3 color = ambient + Lo;

    // HDR tonemapping HDR转LDR 
    color = color / (color + vec3(1.0));

    // gamma correct 伽马纠正 
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color , 1.0);
}
