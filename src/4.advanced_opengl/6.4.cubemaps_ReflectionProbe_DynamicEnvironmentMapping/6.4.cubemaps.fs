#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;
 

void main()
{    
    vec3 I = normalize(Position - cameraPos); // 入射角方向 是指向顶点,跟物理书的一样
	
	#if 1
		vec3 R = reflect(I, normalize(Normal));
	#else 
		/*
			空气		1.00
			水			1.33
			冰			1.309
			玻璃		1.52
			钻石		2.42

			注意，如果要想获得物理上精确的结果，我们还需要在光线离开物体的时候再次折射，
			现在我们使用的只是单面折射(Single-side Refraction)
		*/
		float   Refractive_Index = 1.00 / 1.33;
		vec3 R = refract(I, normalize(Normal), Refractive_Index);
	#endif 
	R.z = -R.z ;
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
}
