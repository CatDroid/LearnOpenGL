#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    vec3 col = texture(screenTexture, TexCoords).rgb;
	vec3 correctGamma = pow(col, vec3(1.0/2.2));
    FragColor = vec4(correctGamma, 1.0);
} 