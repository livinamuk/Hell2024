#version 460 core
out vec4 FragColor;

/*struct Material 
{
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
};*/ 

struct Light {
    vec3 color;
    vec3 position;
};

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform vec3 viewPos;

//uniform Material material;
uniform Light light;

uniform sampler2D yourTexture;

void main() {
    vec3 objectColor = texture(yourTexture, TexCoord).rgb;
	
	// ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * light.color;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * light.color;  
        
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);

}