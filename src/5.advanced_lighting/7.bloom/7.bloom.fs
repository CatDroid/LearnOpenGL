#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;
// 一个着色器使用一个布局location修饰符，fragment就会写入对应的颜色缓冲
// 使用多个像素着色器输出的必要条件是，有多个颜色缓冲附加到了当前绑定的帧缓冲对象上

in VS_OUT { // 接口块
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

struct Light {
    vec3 Position;
    vec3 Color;
};

uniform Light lights[4]; //  setParamter("light[0].Postion", vec3)
uniform sampler2D diffuseTexture;
uniform vec3 viewPos;

void main()
{           
    vec3 color = texture(diffuseTexture, fs_in.TexCoords).rgb;
    vec3 normal = normalize(fs_in.Normal);

    // ambient
    vec3 ambient = 0.0 * color;

    // lighting
    vec3 lighting = vec3(0.0);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    for(int i = 0; i < 4; i++)
    {
        vec3 lightDir = normalize(lights[i].Position - fs_in.FragPos);
        float diff = max(dot(lightDir, normal), 0.0);

        vec3 result = lights[i].Color * diff * color;  // 光源*phong散射参数*albedo颜色
		
        // 点光源的衰减 使用二次方--因为有伽马纠正
        float distance = length(fs_in.FragPos - lights[i].Position);
        result *= 1.0 / (distance * distance);
        lighting += result;           
    }

    vec3 result = ambient + lighting;

	// MRT 亮度/灰度是否大于阈值, 是的话,把颜色输出到slot2 slot1还是片元颜色
    // check whether result is higher than some threshold, if so, output as bloom threshold color
    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        BrightColor = vec4(result, 1.0); // result可以是大于1.0  alpha都是1.0
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0); // 注意alpha都是1.0  

    FragColor = vec4(result, 1.0);
}
