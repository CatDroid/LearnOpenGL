#version 330 core
layout (location = 0) out vec3 gPosition;  // MRT多路输出 
layout (location = 1) out vec3 gNormal;  // 注意 gPosition gNormal  都是浮点纹理 GL_RGBA16F + GL_RGBA + GL_FLOAT
layout (location = 2) out vec4 gAlbedoSpec; //  gAlbedoSpec 是个定点纹理   GL_RGBA + GL_RGBA + GL_UNSIGN_BYTE

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