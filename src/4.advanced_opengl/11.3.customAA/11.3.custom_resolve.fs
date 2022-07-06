#version 330 core
out vec4 FragColor;

in VS_OUT
{
   vec2 texCoords ;
} vs_in;

uniform sampler2DMS myTexture;
//uniform int viewport_width;
//uniform int viewport_height;

void main()
{
    // texelFetch  必须使用整数 

	// ivec2 vpCoords = ivec2(viewport_width, viewport_height);
	//	vpCoords.x = int(vpCoords.x * texCoord.x); 
	//	vpCoords.y = int(vpCoords.y * texCoord.y);

	ivec2 vpCoords = ivec2(gl_FragCoord.x, gl_FragCoord.y);

	vec4 sample1 = texelFetch(myTexture, vpCoords, 0);
	vec4 sample2 = texelFetch(myTexture, vpCoords, 1);
	vec4 sample3 = texelFetch(myTexture, vpCoords, 2);
	vec4 sample4 = texelFetch(myTexture, vpCoords, 3);

	FragColor = (sample1 + sample2 + sample3 + sample4) / 4.0f;

} 