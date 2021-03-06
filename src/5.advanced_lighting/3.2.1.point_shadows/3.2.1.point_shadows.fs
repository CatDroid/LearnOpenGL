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

float ShadowCalculation(vec3 fragPos)
{
    // 从光源指向表面点(世界坐标系)
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - lightPos;

	// 立方体贴图进行采样 的 方向向量不需要是单位向量，所以无需对它进行标准化
    // ise the fragment to light vector to sample from the depth map    
    float closestDepth = texture(depthMap, fragToLight).r;

	// 生成的时候归一化了, 这里恢复
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    closestDepth *= far_plane;

    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);

    // test for shadows  阴影偏移，所以才能避免阴影失真
    float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;        
    
	// 把该点采样的深度纹理显示出来, 结果是一个灰度场景，每个颜色代表着场景的线性深度值
	// 1.0 白色代表距离远  在阴影下的灰度, 应该跟挡住他的物体灰度一样
	// display closestDepth as debug (to visualize depth cubemap)
    //FragColor = vec4(vec3(closestDepth / far_plane), 1.0);    
        
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