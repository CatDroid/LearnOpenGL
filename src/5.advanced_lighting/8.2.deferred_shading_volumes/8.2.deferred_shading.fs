#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;

struct Light {
    vec3 Position;
    vec3 Color;
    
    float Linear;
    float Quadratic;
    float Radius;
};
const int NR_LIGHTS = 32;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;

// WebGL和Opengl ES 2.0，缺少isnan是个问题
// gles 3.0 
// genBtype isnan(genFDType x)   判断x是否是NaN
// genBtype isinf(genFDType x) 	判断x是否是无穷大
bool isnan( float val )
{
	return (val < 0.0 || 0.0 < val || val == 0.0) ? false : true;
}

void main()
{             
    // retrieve data from gbuffer
    vec3 FragPos = texture(gPosition, TexCoords).rgb;
    vec3 Normal = texture(gNormal, TexCoords).rgb;
    vec3 Diffuse = texture(gAlbedoSpec, TexCoords).rgb;
    float Specular = texture(gAlbedoSpec, TexCoords).a;

	#if 0 
	if (abs(Normal.x) < 1e-6 
			&& abs(Normal.y) < 1e-6   
			&& abs(Normal.z) < 1e-6)
	{
		#if 0
			vec4 clearColor = vec4(1.0, 1.0, 1.0, 1.0); 
			FragColor = clearColor;
			return ;
		#else
			discard; // discard不是函数; 这里就可以直接用glClearColor设置的值
		#endif  
	}
	#endif 
    
    // then calculate lighting as usual
    vec3 lighting  = Diffuse * 0.1; // hard-coded ambient component
    vec3 viewDir  = normalize(viewPos - FragPos);
    for(int i = 0; i < NR_LIGHTS; ++i)
    {
        // calculate distance between light source and current fragment
        float distance = length(lights[i].Position - FragPos);
		// if (distance2 <= radius2) // 常见优化是省去平方根,改为存储平方半径
        if(distance < lights[i].Radius)  
        {
			/*
				光体积:
					只有在光体积半径内(暗的亮度阈值是5/256),才计算光照

				实际不可行:

					GPU 和 GLSL 在优化循环和分支方面非常糟糕。
					原因是 GPU 上的着色器执行是高度并行的，
					并且大多数架构都要求对于大量线程，它们需要运行完全相同的着色器代码以使其高效。
					这通常意味着运行着色器会执行 if 语句的所有分支，
					以确保着色器运行对于该组线程是相同的，
					从而使我们之前的半径检查优化完全无用；
					我们仍然会计算所有光源的光照！
			*/


            // diffuse
            vec3 lightDir = normalize(lights[i].Position - FragPos);
            vec3 diffuse = max(dot(Normal, lightDir), 0.0) * Diffuse * lights[i].Color;
            // specular
            vec3 halfwayDir = normalize(lightDir + viewDir);  
            float spec = pow(max(dot(Normal, halfwayDir), 0.0), 16.0);
            vec3 specular = lights[i].Color * spec * Specular;
            // attenuation
            float attenuation = 1.0 / (1.0 + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
            diffuse *= attenuation;
            specular *= attenuation;
            lighting += diffuse + specular;
        }
    }    
    FragColor = vec4(lighting, 1.0);
}
