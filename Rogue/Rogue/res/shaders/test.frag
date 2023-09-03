#version 420 core

layout (binding = 0) uniform sampler2D tex;

in vec3 Normal;
in vec2 TexCoord;
in vec3 WorldPos;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightRadius;
uniform vec3 viewPos;
uniform vec3 lightingColor;
uniform vec3 baseColor;

float calculateDoomFactor(vec3 fragPos, vec3 camPos, float beginDistance, float scale)
{
    float distanceFromCamera = distance(fragPos, camPos);
    float doomFactor = 1;	
    if (distanceFromCamera> beginDistance) {
        distanceFromCamera -= beginDistance;
        doomFactor = (1 - distanceFromCamera);
        doomFactor *= scale;
        doomFactor = clamp(doomFactor, 0.1, 1.0);
    }
    return doomFactor;
}

float getFogFactor(vec3 fragPos, vec3 camPos) {
    float d = distance(fragPos, camPos);
    float b = 0.10;
    return 1.0 - exp( -d * b );
}

float inverseFalloff(float x){
    // 10 is a good number, but you can also try "iMouse.y" to test values
    float const1 = 10.0;
    float xSq = x*x;
    return (1.0-xSq)/(const1*xSq+1.0);
}


void main()
{
    //vec3 baseColor = texture(tex, TexCoord).rgb; 

    float doom = calculateDoomFactor(WorldPos, viewPos, 3, 1);

    doom = getFogFactor(WorldPos, viewPos);
    doom =  doom;
    
    //vec3 lightPos = vec3(0,2.2,0);
    

    float dist = distance(WorldPos, lightPos);
    //dist *= 2;
    float att = 1.0 / (1.0 + 0.1*dist + 0.01*dist*dist);


    vec3 fogColor = vec3(0.05);
    //vec3 finalColor = mix(baseColor, fogColor, doom) * color * att;
    
  //  FragColor = vec4(finalColor, 1.0f);


    vec3 n = normalize(Normal);
    vec3 l = normalize(lightPos - WorldPos);
    float ndotl = clamp(dot(n, l), 0.0f, 1.0f);
  

    att = smoothstep(lightRadius, 0, length(lightPos - WorldPos));
    //att = inverseFalloff(0.25);

    vec3 ambient = vec3(0.0);
    vec3 lighting = att * lightColor * ndotl + ambient;

    // uniform color
    FragColor = vec4(Normal * 0.5 + 0.5, 1.0f);
    FragColor = vec4(lighting, 1.0f);
    FragColor = vec4(baseColor * lightingColor, 1.0f);
    FragColor = vec4(lightingColor, 1.0f);
   // FragColor = vec4(Normal * 0.5 + 0.5, 1.0f);
    //FragColor = vec4(Normal, 1.0);
    //FragColor = vec4(vec3(ndotl), 1.0f);

    // Include fog 
    //FragColor.rgb = mix(FragColor.rgb, fogColor, doom);
}