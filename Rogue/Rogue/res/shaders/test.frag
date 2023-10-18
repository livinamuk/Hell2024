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
uniform int mode;

uniform mat4 view;

#define MAP_WIDTH   32
#define MAP_HEIGHT  22
#define MAP_DEPTH   36
#define VOXEL_SIZE  0.2
#define PROPOGATION_SPACING 1.0
#define PROBE_SPACING  VOXEL_SIZE * PROPOGATION_SPACING

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
    //float bias = 0.0075;
    vec3 lightDir = fragPos - lightPos;
    float bias = max(0.0125 * (1.0 - dot(Normal, lightDir)), 0.00125);
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

vec3 GetProbe(vec3 fragWorldPos, ivec3 offset, out float weight) {

    vec3 gridCoords = fragWorldPos / PROBE_SPACING;
    ivec3 base = ivec3(floor(gridCoords));
    vec3 a = gridCoords - base;

    vec3 probe_color = texelFetch(tex, base + offset, 0).rgb;

    vec3 probe_worldPos = (base + offset) * VOXEL_SIZE;

    vec3 v = normalize(probe_worldPos - fragWorldPos); // TODO: no need to normalize if only checking sign
    float vdotn = dot(v, Normal);

    vec3 weights = mix(1. - a, a, offset);

    if(vdotn > 0 && probe_color != vec3(-1))
        weight = weights.x * weights.y * weights.z;
    else
        weight = 0.;

    return probe_color;
}

void main()
{
    float doom = calculateDoomFactor(WorldPos, viewPos, 4, 1);
    doom = getFogFactor(WorldPos, viewPos);
    
    // uniform color
    //FragColor = vec4(Normal * 0.5 + 0.5, 1.0f);
    //FragColor = vec4(baseColor * lightingColor, 1.0f);

    float worldSpaceMapWidth = MAP_WIDTH * VOXEL_SIZE;
    float worldSpaceMapHeight = MAP_HEIGHT * VOXEL_SIZE;
    float worldSpaceMapDepth = MAP_DEPTH * VOXEL_SIZE;

    vec3 propogrationGridOffset = vec3(-0.1);//VOXEL_SIZE * PROPOGATION_SPACING ;/// 2.0;
    vec3 texCoords = (WorldPos - propogrationGridOffset) / vec3(worldSpaceMapWidth, worldSpaceMapHeight, worldSpaceMapDepth);
       
    // Interpolate visible probes
    float w;
    vec3 light;
    float sumW = 0.;
    vec3 indirectLighting = vec3(0.);
    
    light = GetProbe(WorldPos, ivec3(0, 0, 0), w);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 0, 1), w);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 1, 0), w);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 1, 1), w);
    indirectLighting += w * light;
    sumW += w;
     
    light = GetProbe(WorldPos, ivec3(1, 0, 0), w);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 0, 1), w);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 1, 0), w);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 1, 1), w);
    indirectLighting += w * light;
    sumW += w;

    indirectLighting /= sumW;

    // Direct lightihng
    float shadow0 = ShadowCalculation(shadowMap0, lightPosition[0], WorldPos, viewPos);
    vec3 ligthting0 = GetDirectLighting(lightPosition[0], lightColor[0], lightRadius[0], lightStrength[0]);
    float shadow1 = ShadowCalculation(shadowMap1, lightPosition[1], WorldPos, viewPos);
    vec3 ligthting1 = GetDirectLighting(lightPosition[1], lightColor[1], lightRadius[1], lightStrength[1]);
    float shadow2 = ShadowCalculation(shadowMap2, lightPosition[2], WorldPos, viewPos);
    vec3 ligthting2 = GetDirectLighting(lightPosition[2], lightColor[2], lightRadius[2], lightStrength[2]);
    float shadow3 = ShadowCalculation(shadowMap3, lightPosition[3], WorldPos, viewPos);
    vec3 ligthting3 = GetDirectLighting(lightPosition[3], lightColor[3], lightRadius[3], lightStrength[3]);     
    vec3 directLighting = vec3(0);
    directLighting += shadow0 * ligthting0;
    directLighting += shadow1 * ligthting1;
    directLighting += shadow2 * ligthting2;
    directLighting += shadow3 * ligthting3;
        
    // Composite
    vec3 composite = directLighting + indirectLighting * 1;
    
    // Texturing
    vec3 baseColor =  texture2D(basecolorTexture, TexCoord).rgb;
    if (tex_flag == 1 ) {
        composite *= baseColor;
    }
    if (tex_flag == 2 && WorldPos.y < 2.5) {
        composite *= baseColor;
    }
    if (tex_flag == 3 && WorldPos.y < 2.5) {
        composite *= baseColor;
    }

    // Fog
    //vec3 fogColor = vec3(0.050);
    //composite = mix(composite, fogColor, doom);
    
    // Final color
    if (mode == 0) {
        FragColor.rgb = composite;
    }
    if (mode == 1) {
        FragColor.rgb = lightingColor;
    }
    else if (mode == 2) {
        FragColor.rgb = directLighting;
    }
    else if (mode == 3) {
        FragColor.rgb = indirectLighting;
    }

    /*vec3 lightDir2 = (inverse(view) * vec4(0, 0, 1, 0)).xyz;
    lightDir2 = normalize(vec3(0, 1, 0));
    float ndotL = dot(Normal, lightDir2);
     ndotL = dot(lightDir2, Normal);
    FragColor.rgb = vec3(ndotL);*/
}
