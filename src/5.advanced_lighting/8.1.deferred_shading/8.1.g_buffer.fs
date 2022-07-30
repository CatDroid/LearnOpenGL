#version 330 core
layout (location = 0) out vec3 gPosition;  // MRT多路输出 
layout (location = 1) out vec3 gNormal;  // 注意 gPosition gNormal  都是浮点纹理 GL_RGBA16F + GL_RGBA + GL_FLOAT
layout (location = 2) out vec4 gAlbedoSpec; //  gAlbedoSpec 是个定点纹理   GL_RGBA + GL_RGBA + GL_UNSIGN_BYTE

/*
Unsized Internal Format
GL_RGB                         GL_RGB	    GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5	 
GL_RGBA	                     GL_RGBA	    GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_5_5_5_1

Sized Internal Format        Format       Type
GL_RGBA8	                       GL_RGBA	 GL_UNSIGNED_BYTE 
GL_SRGB8_ALPHA8	       GL_RGBA    GL_UNSIGNED_BYTE

GL_RGBA16F		              GL_RGBA     GL_HALF_FLOAT GL_FLOAT // 这里可以是FLOAT
GL_RGBA32F		              GL_RGBA     GL_FLOAT
GL_RGB565                     GL_RGB       GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5
*/
in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

void main()
{    
    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos;
    // also store the per-fragment normals into the gbuffer
    gNormal = normalize(Normal);
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(texture_diffuse1, TexCoords).rgb;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = texture(texture_specular1, TexCoords).r;
}