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

// Parallax Mapping with Offset Limiting 有偏移量限制的视差贴图
// Steep Parallax Mapping 陡峭视差映射  1.分层 多次采样 2.视角调整样本数量  -- 视角水平看陡坡 锯齿效果以及图层之间有明显的断层，提高样本的数目
// Parallax Occlusion Mapping 视差遮蔽映射 更经常使用 --  不是用触碰的第一个深度层的纹理坐标，而是在触碰之前和之后，在深度层之间进行线性插值
// Relief Parallax Mapping 浮雕视差贴图  比 视差遮蔽映射 更精确一些 但 性能开销更多

/*
 
 视差映射是增强场景细节的一种很好的技术，但在使用它时确实带有一些您必须考虑的伪影(artifacts)。
 大多数情况下，视差映射用于"地板或类似墙壁的表面"，
 
 在这些表面上不太容易确定determine表面的轮廓，并且视角通常大致垂直于表面。
 这样一来，视差映射的伪影(artifacts)就不会那么明显，
 并使其成为增强对象细节的一种非常有趣的技术。
 
 
 Parallax Occlusion Mapping 提供了令人惊讶的好结果，
 尽管一些轻微的伪影(artifacts)和锯齿(aliasing)问题仍然可见，
 但它通常是一个很好的折衷方案，
 只有在大幅放大或查看非常陡峭的角度时才真正可见。
 
 */

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    // number of depth layers
    const float minLayers = 8;
    const float maxLayers = 32;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir))); // viewDir是切线空间的
    
    
    // calculate the size of each layer
    float layerDepth = 1.0 / numLayers;

    
    // the amount to shift the texture coordinates per layer (from vector P)
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers; 
  
  
    // get initial values
    // depth of current layer
    float currentLayerDepth = 0.0;
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = texture(depthMap, currentTexCoords).r;
      
    
    while(currentLayerDepth < currentDepthMapValue)
    {
        // shift texture coordinates along direction of P
        currentTexCoords -= deltaTexCoords;
        
        
        // get depthmap value at current texture coordinates
        currentDepthMapValue = texture(depthMap, currentTexCoords).r;  
        // get depth of next layer
        currentLayerDepth += layerDepth;  
    }
    
    // 在找到 与(位移)表面几何(displaced)surface geometry相交后的深度层 之后
    // 获取 相交前深度层的纹理坐标
    
    
    // get texture coordinates before collision (reverse operations)
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
    float beforeDepthMapValue = texture(depthMap, prevTexCoords).r;
    float beforeLayerDepth = (currentLayerDepth - layerDepth);
    
    // get depth after and before collision for linear interpolation
    float afterDepth  = currentDepthMapValue - currentLayerDepth;  // 相交后的采样深度 和 相交后的层深度 的差 负数
    //float beforeDepth = beforeDepthMapValue - currentLayerDepth + layerDepth;
    float beforeDepth = beforeDepthMapValue - beforeLayerDepth;   // 相交后的采样深度 和 相交后的层深度 的差 正数
 
    // interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth); // -afterDepth/(-afterDepth+beforeDepth) 近似看成相似三角形 
    
    // 线性插值是两层纹理坐标之间的基本插值
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    // 返回最终的插值纹理坐标
    return finalTexCoords;
}

void main()
{           
    // offset texture coordinates with Parallax Mapping
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec2 texCoords = fs_in.TexCoords;
    
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
