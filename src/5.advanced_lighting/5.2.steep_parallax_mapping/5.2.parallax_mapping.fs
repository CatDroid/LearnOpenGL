#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D depthMap;

uniform float heightScale;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    // 根据我们看向表面的角度调整样本数量
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    
    
    // 每一层的深度 z轴 0到1 分成numLayers层 深度
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;
    
    
    // viewDir.xy / viewDir.z 相当于深度是整个高度场/深度场 也就是视线从场的一边到达另外一边的xy offset作为变化总量
    // viewDir.xy * heightScale; // 这个只是把viewDir单位向量的xy 乘以 scale 作为总变化量，有偏移量限制的视差贴图（Parallax Mapping with Offset Limiting）
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    
    // 每次纹理坐标增加量
    vec2 deltaTexCoords = P / numLayers;
  
    // get initial values
    // depth of current layer
    float currentLayerDepth = 0.0;
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(depthMap, currentTexCoords).r;
      
      
    // 循环 每一层 获取 深度图的值 和 当前P向量深度 对比, 如果当前P向量的深度(currentLayerDepth)要小, 说明还在物体表面上方,继续
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        // 由于ViewDir的方向跟P方向是相反
        currentTexCoords -= deltaTexCoords;
        
        // 获取新一层的深度图的深度 和 计算这层P向量的深度
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(depthMap, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // 返回已经偏移的纹理坐标
    return currentTexCoords;
}

void main()
{           
    // offset texture coordinates with Parallax Mapping
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec2 texCoords = fs_in.TexCoords;
    
    // 视差贴图--修改纹理坐标
    texCoords = ParallaxMapping(fs_in.TexCoords,  viewDir);       
    if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;

    // obtain normal from normal map
    vec3 normal = texture(normalMap, texCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);   
   
    // get diffuse color
    vec3 color = texture(diffuseMap, texCoords).rgb;
    // ambient
    vec3 ambient = 0.1 * color;
    // diffuse
    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    // specular    
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    vec3 specular = vec3(0.2) * spec;
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
