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

// lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

uniform vec3 camPos;

// debug 
uniform bool fixAmbient; // false:ibl-ambient; true: fix ambient
uniform bool roughFresnel;// false: fresnelSchlick; true: IBL漫反射的菲涅尔因子会考虑粗糙度
uniform bool n_normalize;// false: 插值变量Normal 不做归一化

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
//  Fresnel-Schlick 方程中加入粗糙度项
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
	// 最大值变成了是光滑度 或者 F0 
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}   

// ----------------------------------------------------------------------------
void main()
{		
    vec3 N = Normal;
	if (n_normalize) 
	{
		N = normalize(Normal); // Fix: 做归一化之后效果不一样
	}
	/*
	else 
	{
		if(gl_FragCoord.x < 400.0)
		{
			N = normalize(Normal);
		}
		else 
		{
		
		}
	}
	*/
	 
    vec3 V = normalize(camPos - WorldPos);
    //vec3 R = reflect(-V, N); 

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);    
		// 注: 以受粗糙度影响的微表面半向量作为菲涅耳公式的输入
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);        
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
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
        kD *= 1.0 - metallic;	                
            
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    
	// 间接光照 --- 也包括漫反射和镜面反射两部分
	//                  分割版的反射方程---漫反射部分   kD * irradiance * albedo; 

	// 这里的irradiance是预计算的 间接漫反射  的辐照率(所有方向上的间接漫反射的黎曼和)

	// 间接漫反射和间接镜面反射, 视为环境光

	// 注: 这里重新再计算一次菲涅尔公式  得到间接光的 反射率 和 漫反射率/折射率
	// 注: 这里菲涅尔公式 用的是表面的法线N( 上面直接光用的是半向量H ), 来模拟菲涅耳效应
	//     (环境光，来自半球内围绕法线 N 的所有方向，因此没有一个确定的半向量)
	// 注: 由于没有考虑表面的粗糙度，间接光的菲涅耳反射率总是会相对较高, 在粗糙非金属表面上看起来有点过强，
	//     但我们是期望较粗糙的表面在边缘反射较弱(使用N,V计算菲涅尔反射率,导致非金属粗糙球体边界有明显白色(金属没有漫反射))
    // ambient lighting (we now use IBL as the ambient term)
    vec3 kS = roughFresnel? 
					fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness) :
					fresnelSchlick(max(dot(N, V), 0.0), F0);
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;	 // 金属是没有漫反射的(这里就变成不贡献环境光了)
    vec3 irradiance = texture(irradianceMap, N).rgb; // 辐照度图 
    vec3 diffuse      = irradiance * albedo;
    vec3 ambient = (kD * diffuse) * ao;
    // vec3 ambient = vec3(0.002);

	if (fixAmbient)
	{
		vec3 ambientFix = vec3(0.03) * albedo * ao;
		ambient = ambientFix;
	}
	
    
    vec3 color = ambient + Lo; // 环境光(IBL-间接光漫反射)

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color , 1.0);
}