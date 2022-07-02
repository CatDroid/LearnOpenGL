#version 330 core
out vec4 FragColor;

in GS_OUT 
{
	vec3 fColor ;
} gs_in ;

void main()
{
    FragColor = vec4(gs_in.fColor, 1.0); //vec4(1.0, 1.0, 0.0, 1.0);
}

