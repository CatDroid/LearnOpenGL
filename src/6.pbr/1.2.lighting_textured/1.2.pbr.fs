#version 330 core
out vec4 FragColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

// material parameters
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;

// lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

uniform vec3 camPos;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
// Easy trick to get tangent-normals to world-space to keep PBR code simplified.
// Don't worry if you don't get what's going on; you generally want to do normal 
// mapping the usual way for performance anways; I do plan make a note of this 
// technique somewhere later in the normal mapping tutorial.
/*
	主流的球体顶点生成有两种方法:
		这里采用的是UVSphere方法，还有 IcoSpher方法

		genType dFdx(genType p); 
			dFdx = dF/dx   d(p(x,y))/dx
			用于计算任何值相对于屏幕空间坐标的变化率(偏导数(partial derivative))
			glsl所有版本都有这个的, ddx and ddy in HLSL
			p: 指定对其进行偏导数的表达式
			p(x,y): 在屏幕空间坐标系中坐标(x，y)的片元(像素)上的某一个变量(坐标向量 颜色等等)
			
			只能用在ps(片段着色器指令)
	   
		dFdxFine 和 dFdyFine(glsl 4.5) 
			基于当前片段及其直接邻居的p值使用局部差分计算导数。

	   dFdxCoarse 和 dFdyCoarse(glsl 4.5) 
			使用基于当前片段邻居的 p 值的局部差分计算导数，并且可能但不一定包括当前片段的值。
			也就是说，
			在给定区域内，实现可以计算比相应的 dFdxFine 和 dFdyFine 函数所允许的更少的唯一位置的导数。

		dFdx/dFdy 返回 dFdxCoarse 或 dFdxFine/ dFdyCoarse 或 dFdyFine 
			实现可以根据性能
			或 API GL_FRAGMENT_SHADER_DERIVATIVE_HINT 提示的值等因素
			来选择要执行的计算。(#extension GL_OES_standard_derivatives : enable)
			????
			高阶导数（例如 dFdx(dFdx(n))）的表达式具有未定义的结果
			混合阶导数（例如 dFdx(dFdy(n))）也是如此
			这里假设表达式 p 是连续的
			因此，通过非均匀控制流评估的表达式(expressions evaluated via non-uniform control flow)
			可能是未定义的 
			????

		原理:
			http://www.aclockworkberry.com/shader-derivative-functions/
			http://hacksoflife.blogspot.com/2009/11/per-pixel-tangent-space-normal-mapping.html
			三角形栅格化期间, GPU会同时跑片元着色器的多个实例
			但并不是一个pixel一个pixel去执行的, 而是将其组织在2x2的一组pixels块中并行执行
			偏导数就是通过像素块中的变量的差值(变化率)而计算出来的
			dFdx表示的是像素块中右边像素的值减去素块中左边像素的值
			dFdy表示的是下面像素的值减去上面像素的值
			dFdx(p(x,y)) = p(x+1, y) - p(x,y) 
			dFdy(p(x,y)) = p(x, y+1) - p(x,y)  ?? 下面-当前 ?? -- 应该是上面减去下面
			
			可以为片段着色器中的每个变量评估导数
			导数函数是纹理 mipmap 实现的基础
			?? 当存在某种对屏幕空间坐标的依赖时(比如 渲染具有屏幕像素同一厚度的线框边缘时)dFdx有用

			在纹理采样期间使用导数来选择最佳的 mipmap 级别
			纹理坐标相对于屏幕坐标的变化率用于选择mipmap； 
			导数越大，mipmap 级别越大（mipmap 大小越小）

			导数可用于在片段着色器中计算当前三角形的面法线(面法线会导致茶壶或者圆球,光照效果明显分块)
			normalize( cross(dFdx(pos), dFdy(pos)) );
			dFdx(pos)和dFdy(pos) 两个偏导数 就是 三角形表面(三个片段/点)的两个向量
			叉积的顺序：作为 OpenGL 坐标系左手(至少在屏幕空间中工作时，这是片段着色器工作的上下文)
			水平导数向量 始终定向右 和 垂直导数向量 向下(???应该向上,opengl屏幕空间y轴向上,原点在左下角,)
			法线向量的叉积的顺序是水平 x 垂直

			在条件分支的情况下会发生什么？
			在具有 8 个着色器实例的 GPU 内核中执行条件分支(蓝色分支和黄色分支)
			3个实例采用第一个分支(黄色), 5个实例采用第二个分支(蓝色)
			在3个实例黄色分支执行期间, 其他 5 个实例处于非活动状态
			("执行位掩码用于激活/停用执行")
			黄色分支之后, 执行掩码反转, 其余 5 个实例执行蓝色分支
			 
			除了分支的效率和性能损失之外, 分歧还破坏了块中像素之间的同步,从而导致"导数操作未定义"
			这是纹理采样的一个问题, 它需要导数以进行 mipmap 级别选择、各向异性过滤等
			当遇到这样的问题时, 着色器编译器可以"展平分支"（从而避免它）或尝试"重新排列代码移动"纹理读取到分支控制流外部 
			在对纹理进行采样时, 可通过使用"显式导数或 mipmap 级别"来避免此问题
			编译器对该段代码给出以下错误："在流控制中不能有发散梯度操作"
			"cannot have divergent gradient operations inside flow control"
			??UE可以控制编译是否展平分支??[branch] attribute ??

			内部块对齐
			阶跃函数, 步进过渡落在 2×2 像素块的中间(当步长落在奇数像素), 将看到一条" 2 像素"厚度的垂直线
			阶梯过渡位于两个相邻 2×2 像素块的中间(当步长落在偶数像素, 分配2x2块刚好把0和1分开在两边的2x2块),不会看到任何垂直线


			https://stackoverflow.com/questions/16365385/explanation-of-dfdx
			GPU 在同一个程序上以“锁步”方式运行一堆线程，每个线程都有自己的一组寄存器
			为了处理条件分支等，为当前运行的线程组提供了一个“活动掩码”
			掩码中不活动的线程实际上不会运行（因此它们的寄存器不会改变）
			当运行片段程序时，要运行的片段被排列成“四边形”——2x2 4 像素的正方形(??quad 机制??)，
			它们总是在一个线程组中一起运行, 组中的每个线程都知道自己的像素坐标(??屏幕空间??)
			并且可以通过翻转 x（或 y）坐标的最低位轻松找到四边形中相邻像素的坐标(屏幕空间??比如7 0x111 -- 0x110 ->6 ??)
			当 GPU 执行 DDX 或 DDY 指令时，它会查看相邻像素的"线程寄存器"并与当前像素的值相减
			从低位（最低位 0）开始 减去 较高坐标的值（最低位 1 ) ?? 比如 屏幕坐标7(0x111)-屏幕坐标6(0x111)

			如果您在条件分支中使用 dFdx 或 dFdy，这会产生影响
			如果 quad 中的一个线程处于活动状态而另一个不处于活动状态，
			GPU 仍将查看非活动线程的寄存器，该寄存器可能具有任何旧值在其中，
			所以结果可能是任何东西。

			quad 机制是否意味着每对"水平像素对" dFdx 都有一个值：是的

			https://community.khronos.org/t/how-are-dfdx-and-dfdy-functions-implemented/66388/7
			永远不会在单个片段上执行着色器； 即使三角形只生成一个片段，
			它仍然会在 2x2 块上运行它。 系统只是丢弃了三个无用的值。

*/
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t); // .s .t 
    vec3 B  = -normalize(cross(N, T)); 

	/*
	    f = 1 /  (deltaU1*deltaV2 - deltaU2*deltaV1)
        |Tx, Ty, Tz | = f * | deltaV2,  -deltaV1 | * | E1x, E1y, E1z |
        |Bx, By,Bz |         | -deltaU2, deltaU1  |    | E2x, E2y, E2z |
		
		{Tx, Ty, Tz} = f* ( deltaV2*{E1x, E1y, E1z} + (-deltaV1) * {E2x, E2y, E2z} )
		
		V1是 点2到点1 (或者认为是屏幕X方向上)
		V2是 点3到点1 (或者认为是屏幕Y方向上)

		由于f只是个标量, T向量最后还需要正交化normalize, 所以不乘以f
		但这里要注意，f的分母如果是0, 那么就无解了! 也就是3个点在UV贴图的一条直接上,或者一个点上
	
		负号是考虑UV纹理V方向上下反了, deltaV2和 deltaV1 要取反, 相当于计算出来的T反了
	*/

    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);// 把切线空间的法线(纹理) 转换到世界坐标系 
}
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
void main()
{		
	//  反射率(albedo)纹理
	//			在美术人员创建是在sRGB空间
	//			需要在光照计算之前先把他们转换到线性空间
	//		    -- 可以这么理解, 美术在电脑上选择"半红", 对应的数字不会是(0.5,0,0), 而应该是SRGB空间的"0.5^0.45~=0.7"
	//             这样经过显示gamma(^2.2) 之后就是"半红"(0.5,0,0), 所以说设计师是在sRGB空间制作的
	//  环境光遮蔽贴图(ambient occlusion maps)
	//			一般 也需要我们转换到线性空间(不过这里用的是白色图)
	//  金属度(Metallic)和粗糙度(Roughness)贴图
	//          大多数时间都会保证在线性空间(这里用的金属度会是0.0到1.0之间的值)

	// 不再用uniform参数, 改成逐片段的参数  
    vec3 albedo     = pow(texture(albedoMap, TexCoords).rgb, vec3(2.2));
    float metallic  = texture(metallicMap, TexCoords).r; // 只有R通道  金属度/粗糙度/AO都是灰度值(标量)
    float roughness = texture(roughnessMap, TexCoords).r;
    float ao        = texture(aoMap, TexCoords).r;

    vec3 N = getNormalFromMap(); // 修改成 法线贴图 
    vec3 V = normalize(camPos - WorldPos);

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
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
		// 对于非金属来说(metalic=0), 由于F很少, 金属工作流中为0.04(常数)
		// 所以漫反射就很大 1.0 - 0.04 = 0.96, 所以会在漫反射中显示albedo的颜色(最后乘上光线颜色)
		// 对于金属来说, 漫反射为0(折射后全部吸收), 颜色由F来确定,D和G提供标量控制大小(最后乘上光线颜色)
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
		// 金属表面在直接照明环境中往往看起来太暗，因为它们没有漫反射
		// 考虑到环境的镜面环境光照后，它们看起来确实更正确
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }   
    
    // ambient lighting (note that the next IBL tutorial will replace 
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.03) * albedo * ao; // ao是白图 所以是1.0
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    FragColor = vec4(color, 1.0);
}