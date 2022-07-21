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
uniform bool disableHeightMap; // 不使用视差纹理, 看下整体偏移的效果

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{ 
	/* 视差: 
			视差parallax是指从"两个不同位置"观察"同一个物体"时，此物体在视野中的位置变化与"差异"
			比如望向一个砖头墙面, 浅视角应该看到砖头和砖头挨着，高视角看就会看到砖头间的缝隙
	
			视差是由相对于观察者的"透视投影"引起的。 
			所以我们必须牢记这一点来移动纹理坐标。 
			这意味着我们必须根据视图方向移动坐标，这对于每个片段都是不同的。

			两篇文章 视差贴图 
			https://catlikecoding.com/unity/tutorials/rendering/part-20/
			https://zhuanlan.zhihu.com/p/309620273 

			hhl:理解 表面贴图,有点像是正交相交拍出来的,
			                        或者说在每一处上方拍出来,
									而不是透视相机拍出来一张照片
									所以在渲染的时候,使用透视相机,而又不在表面点的正上方时候,
									就不应该直接采样表面点的当前纹理了
	*/ 
	//
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
	//		这会导致更正确的投影??，但它确实会恶化我们对浅视角的视差效应的伪影
	//		添加 0.42 的偏差来缓解这种情况，不会接近零
	//		viewDir.xy /= (viewDir.z + 0.42); 
    float height =  texture(depthMap, texCoords).r;  
	if (disableHeightMap) 
	{
		height = 1.0; 
	}

	
	
	// 因为使用深度图 viewDir的方向和P是相反的, 所以是 "减"
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