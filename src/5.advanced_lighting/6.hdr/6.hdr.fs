#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform bool hdr;
uniform float exposure;

void main()
{             
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    if(hdr)
    {
        // reinhard
        // vec3 result = hdrColor / (hdrColor + vec3(1.0));
        // exposure
        vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
        // also gamma correct while we're at it       
        result = pow(result, vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
    }
    else
    {
		/* 由于 2D 四边形的输出直接渲染到默认帧缓冲区，
		    所有片段着色器的输出值最终仍将被限制在 0.0 和 1.0 之间，
		    即使我们在浮点颜色纹理中有多个值超过 1.0。

			当我们直接将 HDR 值写入 LDR 输出缓冲区时，
			就好像我们一开始就没有启用 HDR。 
			我们需要做的是在不丢失任何细节的情况下将所有浮点颜色值转换为 0.0 - 1.0 范围。 
			我们需要应用一个称为色调映射的过程。
		*/   
		if (gl_FragCoord.x < 400)
		{
			vec3 result = pow(hdrColor, vec3(1.0 / gamma));
			FragColor = vec4(result, 1.0);
		}
		else 
		{
			FragColor = vec4(hdrColor, 1.0);
		}
    }
}