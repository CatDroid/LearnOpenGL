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

/*

 RayMarching 光线行进
 RayTracing 光线追踪

 ??? 一般的视差贴图, 假设入口点的高度与交叉点的高度相同。但仅当入口点和交叉点实际上具有相同的高度时，这才是正确的 ???
 当偏移量不大并且高度场变化不大时，它仍然可以很好地工作。
 然而，当“偏移变得太大”或“高度变化太快”时，我们最终会得到一个疯狂的猜测，这很可能是错误的。
 这就是导致表面撕裂的伪影的原因。
 
 
 如果我们能弄清楚光线实际击中高度场(height field)的位置(B向量)，那么我们总能找到真正的可见表面点。
 这不能用单个纹理样本来完成。
 我们必须沿着视线小步移动(更新P向量)，每次都对高度场进行采样，直到到达表面。
 这种技术被称为光线行进 raymarching。
 
*/

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
      
    /*
        unity中编译可能有警告?
            警告告诉我们循环中使用了"梯度指令"gradient instructions。这是指我们循环中的纹理采样.
            GPU 必须确定要使用哪个 mipmap 级别--- (MIPMAP的级别 需要根据临近的片元使用的纹理坐标 来确定)
            !!为此它需要比较相邻片段的已使用 UV 坐标(UV coordinates of adjacent fragments)
     
            !!只有当所有片段都执行相同的代码时，它才能做到这一点。
     
            这对我们的循环来说是不可能的，因为它可以提前终止，这可能因片段而异。
     
            因此编译器将展开循环，这意味着它将一直执行所有九个步骤，无论我们的逻辑是否表明我们可以提前停止。
            相反，它使用确定性逻辑在之后选择最终结果。
     
     
            是的，但我们必须摆脱渐变指令 gradient instructions。
            这可以通过我们自己确定 UV 导数并手动控制 mipmap 级别来实现。
     
            (使用导数是一个高级主题，我不会在本教程中介绍)
            
            即使这样，片段也会被并行处理。
            基本上，一起计算的一批片段的性能, 是由需要最多迭代的片段决定的。
            (the performance of a batch of fragments that are computed together is determined by the fragment that requires the most iterations.)
            
            因此，任何潜在的性能提升都是可变的和不可预测的，并且会因 GPU 而异。
            需要进行广泛的测试以确定哪种方法最适合特定硬件。
     
     
     */
      
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
