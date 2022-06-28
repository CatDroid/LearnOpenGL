#version 330 core
out vec4 FragColor;

in vec3 TexCoords;
in float Zc;
in float Ze;
uniform samplerCube skybox;

void main()
{
    //if (Ze <= 0) // Ze<0在相机后面  Wc=-Ze 
    //{
    //    discard;
    //}
    
    FragColor = texture(skybox, TexCoords);
}
