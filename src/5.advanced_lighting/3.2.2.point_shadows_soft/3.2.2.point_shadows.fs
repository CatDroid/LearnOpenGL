#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D diffuseTexture;
uniform samplerCube depthMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform float far_plane;
uniform bool shadows;

/*
	大多数这些采样都是多余的，与其在原始方向向量附近处采样，
	??不如在采样方向向量的垂直方向进行采样更有意义??

	由于没有(简单的)方法来确定哪些子方向是多余的
	我们可以使用的一个技巧是，取一组偏移方向，它们都是大致可分离的
	例如，它们每个指向完全不同的方向。这将大大减少相邻子方向的数量

	下面我们有这样一个最大20个偏移方向的数组:

	由此，我们可以采用PCF算法从sampleOffsetDirections中获取固定数量的样本
	，并使用这些样本对立方体图进行采样。
	这里的优点是，我们需要更少的样本来获得视觉上相似的结果。

	float diskRadius = 0.05;
	fragToLight + sampleOffsetDirections[i] * diskRadius

*/
// array of offset direction for sampling
vec3 gridSamplingDisk[20] = vec3[] // 20 - 6 去掉正方体 每个面的中心点 
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),  // 立方体 正面4个角点 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1), // 立方体 后面4个角点
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0), // 立方体 中部四条楞上中间点 z=0 XOY平面 4个顶点
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1), // y = 0 : XOZ 平面 4个顶点  
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)  // x = 0 : YOZ 平面 4个顶点 
);

float ShadowCalculation(vec3 fragPos)
{
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - lightPos;
    // use the fragment to light vector to sample from the depth map    
    // float closestDepth = texture(depthMap, fragToLight).r;
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    // closestDepth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // test for shadows
    // float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    // float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;
    // PCF
    // float shadow = 0.0;
    // float bias = 0.05; 
    // float samples = 4.0;  // 4*4*4 采样64次 
    // float offset = 0.1; // 每个方向都是从-0.1到0.1 分成 samples 份 
    // for(float x = -offset; x < offset; x += offset / (samples * 0.5)) // = 2.0 *offset / samples
    // {
        // for(float y = -offset; y < offset; y += offset / (samples * 0.5))
        // {
            // for(float z = -offset; z < offset; z += offset / (samples * 0.5))
            // {
                // float closestDepth = texture(depthMap, fragToLight + vec3(x, y, z)).r; // use lightdir to lookup cubemap
                // closestDepth *= far_plane;   // Undo mapping [0;1]
                // if(currentDepth - bias > closestDepth)
                    // shadow += 1.0;
            // }
        // }
    // }
    // shadow /= (samples * samples * samples);
    float shadow = 0.0;
    float bias = 0.15; // ???比上面的偏移要大了
    int samples = 20;

	/*
		在这里应用的另一个有趣的技巧是
		我们可以根据观看者与碎片的距离改变diskRadius
		这样我们就能根据观察者的距离来增加偏移半径了，
		使阴影在远的时候更柔和(半径更加大 更大范围的模糊)，在近的时候更清晰(半径更加小)
	*/

    float viewDistance = length(viewPos - fragPos); // 是视图坐标系 与原点距离
    // float diskRadius = 0.05;  // 为什么除以25.0 ??  far_plane = 25.0f ???
	float diskRadius = (1.0 + (viewDistance / far_plane)) / 25.0; // far_plane 水平最远的距离 有可能大于1? 
    
	for(int i = 0; i < samples; ++i)
    {
	    // 一个偏移量添加到指定的diskRadius, 在fragToLight方向向量周围从立方体贴图里采样
        float closestDepth = texture(depthMap, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= far_plane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
        
    // display closestDepth as debug (to visualize depth cubemap)
    // FragColor = vec4(vec3(closestDepth / far_plane), 1.0);    
        
    return shadow;
}

void main()
{           
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);
    vec3 lightColor = vec3(0.3);
    // ambient
    vec3 ambient = 0.3 * lightColor;
    // diffuse
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * lightColor;    
    // calculate shadow
    float shadow = shadows ? ShadowCalculation(fs_in.FragPos) : 0.0;                      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    FragColor = vec4(lighting, 1.0);
}