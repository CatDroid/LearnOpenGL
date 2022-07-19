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
 
	// 1. 即使是表面的同一点（采样的都是高度都是H(A)）,但是因为视角的不一样，
	//     viewDir.xy/viewDIr.z * height 得到的偏移就不一样
	//
	// 2. viewDir是在切线空间
	//     viewDir.z会在0.0到1.0之间的某处。
	//     当viewDir大致平行于表面时，它的z元素接近于0.0，除法会返回比viewDir垂直于表面的时候更大的P向量。
	//     所以，从本质上，相比正朝向表面，当带有角度地看向平面时，我们会更大程度地缩放P的大小，从而增加纹理坐标的偏移；
	//     这样做在视角上会获得更大的真实度。

	//  有偏移量限制的视差贴图（Parallax Mapping with Offset Limiting）
	//  ---不除以viewDir.z
	//  普通视差贴图
	//  ---除以viewDir.z,  heightScale不能太大 否则超过1了
    float height =  texture(depthMap, texCoords).r;     
    return texCoords - viewDir.xy * (height * heightScale);     // viewDir.xy/viewDir.z     
}

void main()
{           

	/*
		转换到切线空间(纹理空间) 
			1. 当表面被任意旋转以后很难指出从P获取哪一个坐标
			2. "由于切线和双切线向量与表面的纹理坐标指向相同的方向"，
				我们可以将 向量P 的 x 和 y 分量作为纹理坐标偏移量，而不管表面的方向。
			3. 边缘上，纹理坐标超出了0到1的范围进行采样，
			    根据纹理的环绕方式导致了不真实的结果, 所以丢弃

	*/
    // offset texture coordinates with Parallax Mapping
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec2 texCoords = fs_in.TexCoords;

	if (gl_FragCoord.x < 400)
	{
	    texCoords = ParallaxMapping(fs_in.TexCoords,  viewDir);       
		if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
			discard;
	}
    


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