#version 330 core
out vec4 FragColor;

struct Light { // 方向光
    //vec3 position;
    vec3 direction;

    //vec3 ambient;
    //vec3 diffuse;
    //vec3 specular;
};


in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_refl1;
uniform sampler2D texture_diffuse1 ;
uniform sampler2D texture_specular1 ;
uniform sampler2D texture_normal1 ;
uniform samplerCube  skybox; 

uniform float  shininess;
uniform Light light;
uniform vec3 cameraPos;

uniform vec3 Ka ;
uniform vec3 Kd ;
uniform vec3 Ks ;


void main()
{    
	// aiTextureType_AMBIENT 其实是 反射贴图
	// 大部分的模型都不具有完全反射性。我们可以引入反射贴图(Reflection Map)
	// 通过使用反射贴图，我们可以知道模型的哪些部分该以什么强度显示反射。
    vec3 reflective =  texture(texture_refl1, TexCoords).rgb;
      
    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = Kd * diff * texture(texture_diffuse1, TexCoords).rgb;
    
    // specular
    vec3 viewDir = normalize(cameraPos - FragPos);
    //vec3 reflectDir = reflect(-lightDir, norm);
    //float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    //vec3 specular = Ks * spec * texture(texture_specular1, TexCoords).rgb;
        
	// cubemap环境(高光??)反射 
	//vec3 inputDir = normalize(FragPos - cameraPos);
	vec3 reflectDir = reflect(-viewDir,  norm);
	reflectDir.z = -reflectDir.z; 

	vec3 specular =  texture(skybox, reflectDir).rgb * reflective;

        
    vec3 result =   diffuse + specular ;
    
  
    
    FragColor = vec4(result, 1.0);
}
