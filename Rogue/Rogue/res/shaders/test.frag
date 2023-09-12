#version 420 core

//layout (binding = 0) uniform sampler2D tex;
layout (binding = 0) uniform sampler3D tex;
layout (binding = 1) uniform samplerCube shadowMap0;
layout (binding = 2) uniform samplerCube shadowMap1;
layout (binding = 3) uniform samplerCube shadowMap2;
layout (binding = 4) uniform samplerCube shadowMap3;
layout (binding = 5) uniform sampler2D basecolorTexture;

uniform vec3 lightPosition[4];
uniform vec3 lightColor[4];
uniform float lightStrength[4];
uniform float lightRadius[4];

in vec3 Normal;
in vec2 TexCoord;
in vec3 WorldPos;

out vec4 FragColor;
uniform vec3 viewPos;
uniform vec3 camForward;
uniform vec3 lightingColor;
uniform vec3 indirectLightingColor;
uniform vec3 baseColor;

#define MAP_WIDTH   32
#define MAP_HEIGHT  22
#define MAP_DEPTH   36
#define VOXEL_SIZE  0.2
#define PROPOGATION_SPACING 1.0

uniform int tex_flag;

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
    float b = 0.05;
    return 1.0 - exp( -d * b );
}

float inverseFalloff(float x){
    // 10 is a good number, but you can also try "iMouse.y" to test values
    float const1 = 10.0;
    float xSq = x*x;
    return (1.0-xSq)/(const1*xSq+1.0);
}

vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);
  

float ShadowCalculation(samplerCube depthTex, vec3 lightPos, vec3 fragPos, vec3 viewPos)
{
   // fragPos += Normal * 0.5;
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float far_plane = 20.0;

    float shadow = 0.0;
    float bias = 0.0075;
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 2000.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(depthTex, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= far_plane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
    return 1 - shadow;
}

vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength) {
    float dist = distance(WorldPos, lightPos);
    //float att = 1.0 / (1.0 + 0.1 * dist + 0.01 * dist * dist);
    float att = smoothstep(radius, 0.0, length(lightPos - WorldPos));
    vec3 n = normalize(Normal);
    vec3 l = normalize(lightPos - WorldPos);
    float ndotl = clamp(dot(n, l), 0.0f, 1.0f);
    return vec3(lightColor) * att * strength * ndotl * 2    ;
}



void main()
{
    float doom = calculateDoomFactor(WorldPos, viewPos, 4, 1);
    doom = getFogFactor(WorldPos, viewPos);
    
    // uniform color
    FragColor = vec4(Normal * 0.5 + 0.5, 1.0f);
    FragColor = vec4(baseColor * lightingColor, 1.0f);
    //FragColor.rgb += clamp(indirectLightingColor * 1.0, 0, 1);

    float worldSpaceMapWidth = MAP_WIDTH * VOXEL_SIZE;
    float worldSpaceMapHeight = MAP_HEIGHT * VOXEL_SIZE;
    float worldSpaceMapDepth = MAP_DEPTH * VOXEL_SIZE;

    vec3 propogrationGridOffset = vec3(-0.1);//VOXEL_SIZE * PROPOGATION_SPACING ;/// 2.0;
    vec3 texCoords = (WorldPos - propogrationGridOffset) / vec3(worldSpaceMapWidth, worldSpaceMapHeight, worldSpaceMapDepth);

    vec3 indirectLighting = texture(tex, texCoords).rgb; 

    float shadow0 = ShadowCalculation(shadowMap0, lightPosition[0], WorldPos, viewPos);
    vec3 ligthting0 = GetDirectLighting(lightPosition[0], lightColor[0], lightRadius[0], lightStrength[0]);
    float shadow1 = ShadowCalculation(shadowMap1, lightPosition[1], WorldPos, viewPos);
    vec3 ligthting1 = GetDirectLighting(lightPosition[1], lightColor[1], lightRadius[1], lightStrength[1]);
    float shadow2 = ShadowCalculation(shadowMap2, lightPosition[2], WorldPos, viewPos);
    vec3 ligthting2 = GetDirectLighting(lightPosition[2], lightColor[2], lightRadius[2], lightStrength[2]);
    float shadow3 = ShadowCalculation(shadowMap3, lightPosition[3], WorldPos, viewPos);
    vec3 ligthting3 = GetDirectLighting(lightPosition[3], lightColor[3], lightRadius[3], lightStrength[3]);
     
    FragColor.rgb = vec3(0);

    FragColor.rgb += shadow0 * ligthting0;
    FragColor.rgb += shadow1 * ligthting1;
    FragColor.rgb += shadow2 * ligthting2;
    FragColor.rgb += shadow3 * ligthting3;
    
    FragColor.rgb += indirectLighting * 2;

    vec3 baseColor =  texture2D(basecolorTexture, TexCoord).rgb;
  //  baseColor = vec3(1);
    
    if (tex_flag == 1 ) {
        FragColor.rgb *= baseColor;
    }
    
    if (tex_flag == 2 && WorldPos.y < 2.5) {
        FragColor.rgb *= baseColor;
    }
    if (tex_flag == 3 && WorldPos.y < 2.5) {
    FragColor.rgb *= baseColor;
    }
    
    // Apply fog
    vec3 fogColor = vec3(0.050);
    FragColor.rgb = mix(FragColor.rgb, fogColor, doom);

    //   FragColor.rgb = lightingColor;
   //  FragColor.rgb = indirectLighting;
    //FragColor.rgb = baseColor;
//FragColor.rgb = vec3(TexCoord, 0);










  //   FragColor.rgb = texture2D(lightingColor, TexCoord).rgb;

//    FragColor.rgb = WorldPos;
// FragColor = vec4(indirectLighting, 1.0f);
 // FragColor.rgb = Normal;

  //  FragColor.rgb = indirectLighting;

   //  shadow = ShadowCalculation(shadowMap1, lightPosition[1], WorldPos, viewPos);
  //   FragColor.rgb += vec3(shadow);

    // FragColor.rgb = vec3(lightPosition[0]);
    // FragColor.rgb = vec3(viewPos);
     
    //FragColor = vec4(indirectLightingColor, 1.0f);
    //FragColor = vec4((baseColor * lightingColor) + indirectLightingColor, 1.0f);
    //FragColor = vec4(lightingColor, 1.0f);
   // FragColor = vec4(Normal * 0.5 + 0.5, 1.0f);
    //FragColor = vec4(Normal, 1.0);
    
        vec3 n = normalize(Normal);
    vec3 l = normalize( camForward);
    float ndotl = clamp(dot(n, l), 0.0f, 1.0f);
   // FragColor = vec4(vec3(ndotl), 1.0f);

    // Include fog 
   // FragColor.rgb = mix(FragColor.rgb, fogColor, doom);



}
