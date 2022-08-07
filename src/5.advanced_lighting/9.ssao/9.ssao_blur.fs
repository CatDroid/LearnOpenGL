#version 330 core
out float FragColor;

in vec2 TexCoords;

uniform sampler2D ssaoInput;
uniform bool disableBlur; 

void main() 
{
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoInput, 0));
    float result = 0.0;

	if (disableBlur)
	{
		FragColor = texture(ssaoInput, TexCoords).r;
	}
	else 
	{
		// 与噪声纹理维度相同数量 4x4
		for (int x = -2; x < 2; ++x)  // 这里没有包含2 
		{
			for (int y = -2; y < 2; ++y) // 只有偏移 -2 -1 0 1 等 4个
			{
				vec2 offset = vec2(float(x), float(y)) * texelSize;
				result += texture(ssaoInput, TexCoords + offset).r;
			}
		}
		FragColor = result / (4.0 * 4.0); // 16个像素做平均
	}

}  