#version 330 core
out vec4 FragColor;

in MY_COLOR 
{
   vec3 fColor;
} ps_in;

void main()
{
    FragColor = vec4(ps_in.fColor, 1.0);   
    //  FragColor = vec4(1.0, 1.0, 1.0, 1.0); 
}