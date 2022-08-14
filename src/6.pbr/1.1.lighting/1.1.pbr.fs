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

// lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

uniform vec3 camPos;

uniform bool showD;
uniform bool showF;
uniform bool showG;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
// 从统计学上来估算在受到表面粗糙度的影响下，取向与中间向量一致的微平面的数量 
// 只要给定一些粗糙度的参数
// 当表面比较粗糙的时候，微平面的取向方向会更加的随机, 
//        与h向量取向一致的微平面分布在一个大得多的半径范围内
//        同时较低的集中性也会让我们的最终效果显得更加灰暗 (符合能量守恒)
// 当粗糙度很低（表面很光滑）的时候
//        与中间向量取向一致的微平面会高度集中在一个很小的半径范围内
//        最终会生成一个非常明亮的斑点
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	// Disney 的调查并被 Epic Games 采用，
	// 在几何函数和法线分布函数中, 对原始粗糙度值进行平方, 看起来更正确

    float a = roughness*roughness; // 注! 这里做了映射 
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
// 从统计学上近似的求得了微平面间相互遮蔽的比率, 导致光线被遮挡
float GeometrySchlickGGX(float NdotV, float roughness)
{
	// 根据用于直接光照还是IBL光照 对roughness做重映射
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0; // 直接光照 

	// 几何函数是GGX与Schlick-Beckmann近似的结合体，因此又称为Schlick-GGX
	// 
	// 曲线图像是 0到1的 上弯的曲线  NdotV=1代表夹角为0， 
	// roughness 小的时候, NdotV在1到0.5, 返回值几乎在1.0, 但NdotV在0.5之后急速下降(导致有明显边界, 拐点) 
	// roughness 大的时候, 几乎是一条y=x的直线， 变化缓慢, 不会有明显的突然变化
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
// 结果是0到1 , 1.0表示没有微平面阴影 0.0则表示微平面彻底被遮蔽
// 史密斯法(Smith’s method)
//     将观察方向（几何遮蔽(Geometry Obstruction)）和
//     光线方向向量（几何阴影(Geometry Shadowing)）都考虑进去 
//     估算几何部分
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
// 描述的是反射光 对比 折射光 所占的比率(应该是反射光对入射光的比例), 跟材质和视角都有关系

// 掠射角
//     (视线和表面法线的夹角接近90度）菲涅尔现象就越明显，反光就越强
//     可以理解在掠射角上的点, 进入到眼球的, 大部分都是来自反射的光线, 而不是折射的光线(非金属, 光线可逆, 光线的颜色)
// 
// F0 表示平面的基础反射率
//      来源1 -- 折射率计算, 只适合电介质 
//      来源2 -- 预先计算出平面对于"法向"入射的反应(处于0度角)
// 特点:
//     导体或者金属表面而言基础反射率一般是带有色彩的 在0.5和1.0之间变化
//     电介质材质表面的基础反射率都不会高于0.17
//     F0 取最常见的电介质表面的平均值 0.04 
//     金属表面会吸收所有折射光线而没有漫反射, 直接使用 表面颜色纹理 作为 基础反射率
//     对于金属来说, F0比较大, 菲涅尔现象不会太明显(1.0-F0比较小, 受到costTheta影响比较小)
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
	//return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); // clamp?防止黑点??NaN??
}
// ----------------------------------------------------------------------------
void main()
{		
    vec3 N = normalize(Normal);
    vec3 V = normalize(camPos - WorldPos);
	//
	// 计算法向入射的反射率 --- F0 基础反射率 
	//
	// 在 "PBR 金属工作流程"中，我们做了简化假设，
	// 把 F0 设为常数 0.04，对大多数电介质表面在视觉上是正确的
	// 对于 金属表面的 F0由反照率值albedo给出 
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic); 
	// 反射率，(albedo, F0,  Ks, kD, , 漫反射之间的关系)
	//    由电介质均值0.04和反照率albedo, 根据金属度控制到到基础反射率F0,
	//    经过菲涅尔方程(考虑视角)后得到反射率, 并作为kS, kD为1.0-kS
	//    金属没有漫反射, 折射光全部被吸收, 所以kd应该为0 (1.0-metal)
	//    (非金属)漫反射在kD的基础上, 需要乘以albedo(吸收的部分)

    // reflectance equation 反射率方程
    vec3 Lo = vec3(0.0);
	bool debugShow =  (showD||showF||showG);
	int lightNum =debugShow? 1:4;
	float NDF  ;
	float G  ;
	vec3 F  ; // 菲涅尔方程是返回vec3的 代表RGB三种波长的反射率 

	// 假设点光源体积无限小(方向光和聚光灯一样), 对于一个光源, 表面上的一点,该光源只有一个方向(一簇光线)影响
	// 每个光源仅仅只有一个方向上的光线会影响物体表面的辐射率
    for(int i = 0; i < lightNum; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V + L); // 半向量取 视线和光线 之间

		//
		// 辐射率  --- 辐射通量  立体角(球面度, 类似圆心角-弧度)
		//
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);// 平方反比定律(物理学定律, 按照距离源的平方反比而下降)
        vec3 radiance = lightColors[i] * attenuation; // 或用常数线性二次衰减方程(constant-linear-quadratic,)更多控制(物理不正确)

		//
		// 对于每个灯光，我们要计算完整的 Cook-Torrance 镜面反射 BRDF 项
		//
        NDF = DistributionGGX(N, H, roughness);   // 金属度没有关系
        G     = GeometrySmith(N, V, L, roughness);  // 金属度没有关系   
		// 菲涅耳方程:计算镜面反射和漫反射之间的比率(或者表面反射光的程度与折射光的程度)
		//                漫反射 是由于折射, 没被吸收和穿透, 次表面反射形成的
        F     = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0); // 金属度有关,粗糙度无关
        //F     = fresnelSchlick(clamp(dot(N, V), 0.0, 1.0), F0); 
		// 改成N和V的话, 菲涅尔会更加明显,掠射角明显白色
		// 如果是dot(H,V) 那么跟物体的角度(法向量无关,整个圆球都一样)就没有关系了,只跟光线和视线夹角一半和粗糙度有关
		//F     = fresnelSchlick(dot(H, V), F0);

        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
		// + 0.0001 to prevent divide by zero 避免除0 
        vec3 specular = numerator / denominator;
        
        // (kS等价于F)  kS 反射部分占入射光线的比例  kD 折射部分(引起漫反射部分, 剩下的光能会被折射)
        vec3 kS = F;  
        // 为了能量守恒，漫反射光和镜面光不能超过1.0（除非表面发光）； 
	    // 为了保持这种关系，漫反射分量 (kD, 折射比) 应该等于 1.0 - kS
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
		// 将 kD 乘以逆金属度(inverse metalness)，这样"只有非金属具有漫射光"，
		// 对于部分金属(混合了部分金属)，则为线性混合(纯金属没有漫射光, 折射全部被吸收）
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);  // 平面和光线方向不一样, 平面为单位1的时候,在光线方向上的面积为NdotL 

		vec3 f_lambert = albedo / PI;
		vec3 lambertDiffuse = kD * f_lambert;

        // add to outgoing radiance Lo
        Lo += (lambertDiffuse + specular) * radiance * NdotL;  // dw=1 
		// note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
		// fr = kD * f_lambert +  kS * f_cook_torrance
		// ?? 这样说 kS 就是 已经包含在 f_cook_torrance中的 F ??  代码上也把kS=F ？  
	}   
    
	// ------------------------------------------------------------------
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo; // 环境光+辐照度


	// ------------------------------------------------------------------
	// 线性空间和HDR渲染

    // HDR tonemapping  色调映射 / tone or exposure map 曝光控制
	// Reinhard 算子  保留"可能高度变化的辐照度"的高动态范围
	//     形状类似过原点的log函数, 输出不超过1  
	//     输入0~1        占了输出值域的0~0.5,
	//     输入超出1.0    占了输出值域的0.5~1.0)
    color = color / (color + vec3(1.0));

	if (debugShow) // debug的时候
	{
		if (showD)
		{
			color = color*0.00000001 + vec3(NDF, NDF, NDF);
		}
		else if(showF)
		{
			color = color*0.00000001 + F;
		}
		else if(showG)
		{
			// 如果做了伽马校正,  粗糙度在0.2和0.8都看不出教程上的明显变化
			color = color*0.00000001 + vec3(G, G, G);
		}
	}

    // gamma correct 伽马矫正
    color = pow(color, vec3(1.0/2.2)); 



    FragColor = vec4(color, 1.0);
}
