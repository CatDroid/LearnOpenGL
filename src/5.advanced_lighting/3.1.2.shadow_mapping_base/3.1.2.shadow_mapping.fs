#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D diffuseTexture;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragPosLightSpace, vec3 worldNormal, vec3 worldLightDir)
{
    // perform perspective divide ps阶段才做透视除法
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
	// transform to [0,1] range  注意这里把深度从1~-1 也转换到了 1~0 
    projCoords = projCoords * 0.5 + 0.5;
    
	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;  // 只要r分量
    
	// get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
	// 阴影失真(Shadow Acne) ---线条样式--- 
	//       1.阴影贴图受限于分辨率 
	//       2.距离光源比较远
	// 
	// 阴影偏移（shadow bias）
	//		 1. 有一个偏移量的最大值0.05，和一个最小值0.005，
	//      2. 能够根据表面朝向光线的角度更改偏移量
	//          它们是基于表面法线和光照方向的 (1.0 - dot(normal, lightDir)
	//          ---几乎与光源垂直的地方，得到的偏移就很小
	//          ---立方体的侧面这种表面得到的偏移就更大
	//      3. 缺点--悬浮  (Peter Panning)
	//          a. 偏移有可能足够大
	//          b. 遮挡物体比较薄 

    float bias = gl_FragCoord.x > 400.0? 0 : max(0.05 * (1.0 - dot(worldNormal, worldLightDir)), 0.005);
	//float bias = 0.0;

	// check whether current frag pos is in shadow 1.0代表是阴影
    float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;

	
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
    //vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;
	float shininess =  64.0;
    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = spec * lightColor;  
	
    // calculate shadow 阴影的话没有diffuse和specular
	//  (diffuse可能不是0, 表面法线和光源方向可能一致,但被挡住了)
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, normal, lightDir);                      
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;    
    
    FragColor = vec4(lighting, 1.0);
}