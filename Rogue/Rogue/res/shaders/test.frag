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

vec3 GetProbeContribution(vec3 fragWorldPos, int gridDeltaX, int gridDeltaY, int gridDeltaZ) {

    int gridX = int(fragWorldPos.x / PROBE_SPACING);
    int gridY = int(fragWorldPos.y / PROBE_SPACING);
    int gridZ = int(fragWorldPos.z / PROBE_SPACING);

    vec3 color = vec3(0);
    ivec3 probe_gridPos = ivec3(gridX + gridDeltaX, gridY + gridDeltaY, gridZ + gridDeltaZ);
    vec3 probe_color = texelFetch(tex, probe_gridPos, 0).rgb;
    vec3 probe_worldPos = probe_gridPos * VOXEL_SIZE;
    vec3 v = normalize(probe_worldPos - fragWorldPos);
    float vdotn = dot(v, Normal);

    // If probe is in front and NOT black (inside geo)
    if (vdotn > 0 && probe_color != vec3(0)) {
        float dist = distance(probe_worldPos, fragWorldPos); 
        color = probe_color * dist;
    }
    
    return vec3(probe_worldPos  * VOXEL_SIZE );
}

vec3 GetProbeColor(vec3 fragWorldPos, int gridDeltaX, int gridDeltaY, int gridDeltaZ) {

    int gridX = int(fragWorldPos.x / PROBE_SPACING);
    int gridY = int(fragWorldPos.y / PROBE_SPACING);
    int gridZ = int(fragWorldPos.z / PROBE_SPACING);

    vec3 color = vec3(0);
    ivec3 probe_gridPos = ivec3(gridX + gridDeltaX, gridY + gridDeltaY, gridZ + gridDeltaZ);
    vec3 probe_color = texelFetch(tex, probe_gridPos, 0).rgb;
    vec3 probe_worldPos = probe_gridPos * VOXEL_SIZE;
    vec3 v = normalize(probe_worldPos - fragWorldPos);
    float vdotn = dot(v, Normal);

    // If probe is in front and NOT black (inside geo)
    if (vdotn > 0 && probe_color != vec3(0)) {
        color = probe_color;
    }
    return vec3(color);
}

vec3 GetColorMixBetweenTwoProbes(vec3 fragPos, ivec3 probeA, ivec3 probeB, float mixFactor) {
    vec3 colorA = GetProbeColor(fragPos, probeA.x, probeA.y, probeA.z);
    vec3 colorB = GetProbeColor(fragPos, probeB.x, probeB.y, probeB.z);
    if (colorA == vec3(0)) {
        colorA = colorB;
    }
    else if (colorB == vec3(0)) {
        colorB = colorA;
    }
    return mix(colorA, colorB, mixFactor);
}

struct ProbeResult {
    vec4 color;
    float dist;
};

ProbeResult GetProbeResult(vec3 fragWorldPos, float gridDeltaX, float gridDeltaY, float gridDeltaZ) {

    int probeX = int(round((fragWorldPos.x / PROBE_SPACING) + gridDeltaX));
    int probeY = int(round((fragWorldPos.y / PROBE_SPACING) + gridDeltaY));
    int probeZ = int(round((fragWorldPos.z / PROBE_SPACING) + gridDeltaZ));

    ProbeResult result;
    result.dist = 1;
    result.color = vec4(0);

    ivec3 probe_gridPos = ivec3(probeX, probeY, probeZ);
    vec3 probe_color = texelFetch(tex, probe_gridPos, 0).rgb;
    vec3 probe_worldPos = probe_gridPos * VOXEL_SIZE;
    vec3 v = normalize(probe_worldPos - fragWorldPos);
    float vdotn = dot(v, Normal);

    // If probe is in front and NOT black (inside geo)
    if (vdotn > 0 && probe_color != vec3(0)) {    
        result.dist = distance(probe_worldPos, fragWorldPos); 
        result.color.xyz = probe_color;
    }
    return result;
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
   
    //int gridWorldX = int(WorldPos.x / MAP_WIDTH) ;//* MAP_WIDTH;
   // int gridWorldY = int(WorldPos.y / MAP_HEIGHT);// * MAP_HEIGHT;
   // int gridWorldZ = int(WorldPos.z / MAP_DEPTH);// * MAP_DEPTH;


  // vec3 probeE = texture(tex, iTexCoords).rgb; 

 //   texCoords.x -= 0.00001;
 //   texCoords.y += 0.00001;
    texCoords.z += 0.00001;
    
    float texelWidth = 1.0 / MAP_WIDTH;
    float texelHeight = 1.0 / MAP_HEIGHT;
    float texelDepth = 1.0 / MAP_DEPTH;

    vec3 WorldPos3 = WorldPos -  vec3(0.00);

    int gridX = int(WorldPos3.x / PROBE_SPACING) + 0;
    int gridY = int(WorldPos3.y / PROBE_SPACING) + 0;
    int gridZ = int(WorldPos3.z / PROBE_SPACING) + 0;

  //  gridZ = 14;

    float dx = (WorldPos3.x / PROBE_SPACING) - gridX;
    float dy = (WorldPos3.y / PROBE_SPACING) - gridY;
    float dz = (WorldPos3.z / PROBE_SPACING) - gridZ;
    
   // dx = 0;
  //  dy = 0;
    //dz = 0;
   


   
    ivec3 iTexCoords = ivec3(gridX, gridY, gridZ);
    
    vec3 final = vec3(0,0,0);
    ivec3 fragGridCoords = ivec3(gridX, gridY, gridZ);

    ivec3 x0y0z0 = iTexCoords + ivec3(0, 0, 0);
    ivec3 x1y0z0 = iTexCoords + ivec3(1, 0, 0);

    
    ivec3 x0y0z1 = iTexCoords + ivec3(0, 0, 1);

    
    final = GetProbeContribution(WorldPos, -1, 0, 0);
    
    



    float d_x = (WorldPos.x / PROBE_SPACING) - gridX; 
    float d_y = (WorldPos.y / PROBE_SPACING) - gridY; 
    float d_z = (WorldPos.z / PROBE_SPACING) - gridZ; 

    // front facing z
    vec3 zFrontColor;
    vec3 zFrontTopColor = GetColorMixBetweenTwoProbes(WorldPos, ivec3 (0, 1, 1), ivec3(1, 1, 1), d_x);
    vec3 zFrontBottomColor = GetColorMixBetweenTwoProbes(WorldPos, ivec3 (0, 0, 1), ivec3(1, 0, 1), d_x);
    if (zFrontBottomColor == vec3(0)) {
            zFrontBottomColor = zFrontTopColor;
    }
    else if (zFrontTopColor == vec3(0)) {
        zFrontTopColor = zFrontBottomColor;
    }
    zFrontColor = mix(zFrontBottomColor, zFrontTopColor, d_y);

    // back facing z
    vec3 zBackColor;
    vec3 zBackTopColor = GetColorMixBetweenTwoProbes(WorldPos, ivec3 (0, 1, 0), ivec3(1, 1, 0), d_x);
    vec3 zBackBottomColor = GetColorMixBetweenTwoProbes(WorldPos, ivec3 (0, 0, 0), ivec3(1, 0, 0), d_x);
    if (zBackBottomColor == vec3(0)) {
            zBackBottomColor = zBackTopColor;
    }
    else if (zBackTopColor == vec3(0)) {
        zBackTopColor = zBackBottomColor;
    }
    zBackColor = mix(zBackBottomColor, zBackTopColor, d_y);



    if (zBackColor == vec3(0)) {
            zBackColor = zFrontColor;
    }
    else if (zBackTopColor == vec3(0)) {
        zFrontColor = zBackColor;
    }
    final = mix(zBackTopColor, zFrontColor, d_z);


  //  final = zBackColor;





  
    vec3 col_x0y0z1 = GetProbeColor(WorldPos, 0, 0, 1);
    vec3 col_x0y1z1 = GetProbeColor(WorldPos, 0, 1, 1);




    final = vec3(0);
    
    final += mix(col_x0y0z1, vec3(0), d_y);
    final += mix(col_x0y1z1, vec3(0), d_y);





    vec3 probeAB, probeCD, probeEF, probeGH;

    vec3 probeA = GetProbeColor(WorldPos, 0, 0, 0);
    vec3 probeB = GetProbeColor(WorldPos, 1, 0, 0);

    if (probeA == vec3(0))
        probeAB = probeB;
    else if (probeB == vec3(0))
        probeAB = probeA;
    else
        probeAB = mix(probeA, probeB, dx);





    vec3 probeC =  GetProbeColor(WorldPos, 0, 1, 0);
    vec3 probeD =  GetProbeColor(WorldPos, 1, 1, 0);

    if (probeC == vec3(0))
        probeCD = probeD;
    else if (probeD == vec3(0))
        probeCD = probeC;
    else
        probeCD = mix(probeC, probeD, dx);

    vec3 probeE = GetProbeColor(WorldPos, 0, 0, 1);
    vec3 probeF = GetProbeColor(WorldPos, 1, 0, 1);

    if (probeE == vec3(0))
        probeEF = probeF;
    else if (probeF == vec3(0))
        probeEF = probeE;
    else
        probeEF = mix(probeE, probeF, dx);

    vec3 probeG =  GetProbeColor(WorldPos, 0, 1, 1);
    vec3 probeH =  GetProbeColor(WorldPos, 1, 1, 1);

    if (probeG == vec3(0))
        probeGH = probeH;
    else if (probeH == vec3(0))
        probeGH = probeG;
    else
        probeGH = mix(probeG, probeH, dx);

    vec3 frontColor = vec3(1,0,1);
   if (probeEF == vec3(0))
        frontColor = probeGH;
    else if (probeGH == vec3(0))
        frontColor = probeEF;
    else
        frontColor = mix(probeEF, probeGH, dy);
   
   vec3 backColor;
   if (probeAB == vec3(0))
        backColor = probeCD;
    else if (probeCD == vec3(0))
        backColor = probeAB;
    else
        backColor = mix(probeAB, probeCD, dy);




     final = vec3(1,0,0);
    if (backColor == vec3(0))
        final = frontColor;
    else if (frontColor == vec3(0))
        final = backColor;
    else
        final = mix(backColor, frontColor, dz);




    vec3 probeGHlow;
    vec3 probeGlow =  GetProbeColor(WorldPos, 0, -1, 1);
    vec3 probeHlow =  GetProbeColor(WorldPos, 1, -1, 1);

    if (probeGlow == vec3(0))
        probeGHlow = probeHlow;
    else if (probeH == vec3(0))
        probeGHlow = probeGlow;
    else
        probeGHlow = mix(probeGlow, probeHlow, dx);


   //final = probeCD;





   
     ProbeResult probe0 =  GetProbeResult(WorldPos, -0.5, 0.5, 0.5);
     ProbeResult probe1 =  GetProbeResult(WorldPos,  0.5, 0.5, 0.5);
     ProbeResult probe2 =  GetProbeResult(WorldPos, -0.5, -0.5, 0.5);
     ProbeResult probe3 =  GetProbeResult(WorldPos,  0.5, -0.5, 0.5);
     
    /// vec4 weight = normalize(vec4(probe0.dist, probe1.dist, probe2.dist, probe3.dist));
     vec2 weight = normalize(vec2(probe0.dist, probe1.dist));

 //    final = vec3(0);
 //    final += probe0.color.xyz * (1 - weight.x); 
 //    final += probe1.color.xyz * (1 - weight.y); 
  //   final += probe2.color * (1 - weight.b); 
  //   final += probe3.color * (1 - weight.a); 

   //  final = vec3(weight.x + weight.y) - vec3(0.25);
   //  final += probe0.color * (weight.x); 
   //  final += probe1.color * (weight.y); 
     
   //  final = probe0.color;
   //  final = vec3(weight.y);


     vec3 indirectLighting = vec3(final);


























  //   indirectLighting = WorldPos.xyz / VOXEL_SIZE;/ 
//  indirectLighting = vec3(probeCD);
//    indirectLighting = vec3(iTexCoords) * 0.1 ;

 //   indirectLighting = vec3(probeC);
    // closest probe
  //  vec3

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
    vec3 fogColor = vec3(0.050);
    composite = mix(composite, fogColor, doom);
    
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
