#version 330 core
out vec4 FragColor;

struct Light { // 方向光
    //vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1 ;
uniform sampler2D texture_specular1 ;
uniform sampler2D texture_normal1 ;
uniform float  shininess;
uniform Light light;
uniform vec3 viewPos;

uniform vec3 Ka ;
uniform vec3 Kd ;
uniform vec3 Ks ;


void main()
{    
    // ambient
    vec3 ambient = Ka * texture(texture_diffuse1, TexCoords).rgb;
      
    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = Kd * diff * texture(texture_diffuse1, TexCoords).rgb;
    
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = Ks * spec * texture(texture_specular1, TexCoords).rgb;
        
        
    vec3 result = ambient + diffuse + specular ;
    
  
    
    FragColor = vec4(result, 1.0);
}
