#version 330 core
out vec4 FragColor;
in vec3 WorldPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main()
{		
	// The world vector acts as the normal of a tangent surface
    // from the origin, aligned to WorldPos. Given this normal, calculate all
    // incoming radiance of the environment. The result of this radiance
    // is the radiance of light coming from -Normal direction, which is what
    // we use in the PBR shader to sample irradiance.
    vec3 N = normalize(WorldPos);

    vec3 irradiance = vec3(0.0);   
    
    // tangent space calculation from origin point
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
       
    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);

			// 积分改成黎曼和--离散

			// +=  radiance(p, wi) * cos(theta) * sin(theta) * (2π/n1) * ((π/2)/n2)    * ( kd * c / π )

			// 这里最后的结果不包含 kd * c ,  这两个参数跟物体材质相关 

            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples)); // nrSamples = n1*n2;
    
    FragColor = vec4(irradiance, 1.0); 
	// 该方向上的辐照度
	// 反射方程中的 漫发射部分-- 与输出方向(视线方向)无关, 只跟入射方向和表面法线方向有关(跟Phong模型类似)
}
