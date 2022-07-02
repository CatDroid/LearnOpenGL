#version 330 core
out vec4 FragColor;

//in vec3 fColor;

in GS_OUT 
{
   vec3 fColor;
} ps_in;

void main()
{
    FragColor = vec4(ps_in.fColor, 1.0);   
}