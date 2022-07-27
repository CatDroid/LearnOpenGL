#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform bool bloom;
uniform float exposure;

void main()
{             
    const float gamma = 2.2;

    vec3 hdrColor = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;

    if(bloom)
        hdrColor += bloomColor; 
		// additive blending 模糊处理的图像和场景原来的HDR纹理 两个纹理进行混合



    // 色调映射  tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // 伽马滤波器   
    result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}