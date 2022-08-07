#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D texNoise;

uniform vec3 samples[64];

uniform bool disableRandomRotation ;

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 64;
float radius = 0.5;
float bias = 0.025;

// 屏幕的平铺噪声纹理, 会根据屏幕分辨率除以噪声大小的值来决定
// 这样相当于每4x4格子都重复采样, 纹理采样方式要是GL_REPEAT
//
// Q: 如果把4x4的纹理,平铺到800x600的平面上 ??
// A: 纹理坐标0~1-->乘上分辨率(0~800)->除以纹理分辨率(0~400) 
//     这样屏幕的0~4的纹理坐标就是0~1.0 完整的纹理, 4~8 repeat 0~1.0 第二完整的纹理
// 
// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(800.0/4.0, 600.0/4.0);  // 屏幕 = 800x600

uniform mat4 projection;

void main()
{
    // get input for SSAO algorithm
    vec3 fragPos = texture(gPosition, TexCoords).xyz;

    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);

	// texNoise只是用来旋转 切空间 的，里面存储的向量并不是在切空间定义的(?而是相机空间?? z=0,x和y不为0)
	// 也就是切线不用对齐 UV纹理方向的
	// (因为我们使用了一个随机向量来构造切线向量, 不需要逐顶点切线(和双切)向量)
	// texNoise的平铺参数设置为GL_REPEAT，随机的值将会在全屏不断重复
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);
   
    vec3 tangent = !disableRandomRotation? 
				normalize(randomVec - normal * dot(randomVec, normal)):
				vec3(0.5, 0.5, 0.0); // 关闭随机旋转的话, 就用一个固定的切向代替

    vec3 bitangent = cross(normal, tangent);

	// create TBN change-of-basis matrix: from tangent-space to view-space
    mat3 TBN = mat3(tangent, bitangent, normal);

	// 可以调整参数 
	// 1. 采样核心的大小 kernelSize(samples数组长度)
	// 2. 采样半径 radius
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 samplePos = TBN * samples[i]; // from tangent to view-space

        samplePos = fragPos + samplePos * radius;  
		// 从切向空间转换到视图空间, 需要加上顶点坐标(位移)
		// radius 增加(或减少)SSAO的有效取样半径 (radius 标量, 也可以说作用在samples[i]上)
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = projection * offset; // from view to clip-space 透视投影(透视矩阵)
        offset.xyz /= offset.w; // perspective divide					透视除法 
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0 从-1到1转换到0到1(纹理坐标)
        
        // get sample depth  gPosition纹理滤波方式是REPEAT
		// 获取样本位置"从观察者视角第一个不被遮蔽"的深度值
        float sampleDepth = texture(gPosition, offset.xy).z; // get depth value of kernel sample
        /*
			SSAO 实际是相机位置有关的
				在相机A位置看 和 在相机B位置看, 表面的同一个点, 
				由于A位置看, 一些采样点可能被遮挡, 在B位置看就不会
				这样就会导致两个相机计算出来, 同一点的遮蔽强度，就会不一样
		
		*/
        // range check & accumulate
		// abs在radius以内都是1， 在radius以外abs越大越接近0
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
		
		// samplePos.z 是样本顶点的位置(当前点往法线的某个方向凸出一点的位置,其线性深度)
		// sampleDepth 是样本顶点 透视投影后gPostion纹理的值, 也就是邻近点的线性深度 , 
		//                     越小代表越深(远),没有挡住;越大就代表越近,在前面挡住了
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;     
		
		/*
			为原始片段的深度值添加了一个小偏差（在此示例中设置为 0.025）。 
			偏差并不总是必要的，但它有助于在视觉上调整 SSAO 效果
			并解决基于场景复杂性可能出现的粉刺效果。

			每当一个片段被测试为"靠近"表面边缘对齐的环境遮挡时，
			它也会考虑"远离""测试表面"的"表面surface"的深度值； 
			这些值将（错误地）影响遮挡因子。 
			我们可以通过引入范围检查来解决这个问题
			(屏幕空间两个像素, 实际在3D空间中是距离很远的,超过radius)

			只当被测深度值在取样半径内时影响遮蔽因子
			使用smoothstep函数
			如果深度差因此最终取值在radius之间， 
			它们的值将会光滑地根据下面这个曲线插值在0.0和1.0之间

			使用一个在深度值在radius之外就突然移除遮蔽贡献的硬界限范围检测(Hard Cut-off Range Check)，
			我们将会在范围检测应用的地方看见一个明显的(很难看的)边缘

		*/
    }

	// 将遮蔽贡献根据核心的大小标准化  occlusion相当于遮挡的数目  遮挡强度=occlusion / kernelSize
    occlusion = 1.0 - (occlusion / kernelSize);

	// 原来1.0是代表遮挡 , 相当于要环境光照为0, 所以要1.0减去遮挡强度
	//  用来缩放环境光照分量
    
    FragColor = occlusion;
	// 在片段着色器（out float FragColor;）中输出“float”时
	// 因为“FragColor”是一个"float"而不是"vec4"，
	// 所以输出的是："float, undef, undef, undef"
	// 解决?? 实际FBO颜色附件只有GL_RED, 其他通道都不会写入
	// glDisable(GL_BLEND); renderQuad(for SSAO); glEnable(GL_BLEND);
	// oFragColor = vec4(occlusion, 0.0, 0.0, 1.0); 

	// FragColor = pow(occlusion, power);
	// pow幂从而增加它的强
}
