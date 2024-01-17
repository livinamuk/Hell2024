#version 420

struct Light {
	vec3 position;
	vec3 color;
	float strength;
	float radius;
};

layout (location = 0) out vec4 FragColor;
in vec2 TexCoords;

layout (binding = 0) uniform sampler2D basecolorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;
layout (binding = 3) uniform sampler2D depthTexture;
layout (binding = 4) uniform sampler3D propgationGridTexture;
layout (binding = 5) uniform samplerCube shadowMap[16];

layout (binding = 22) uniform samplerCube player1_shadowMap;
layout (binding = 23) uniform samplerCube player2_shadowMap;

uniform mat4 projectionScene;
uniform mat4 projectionWeapon;
uniform mat4 inverseProjectionScene;
uniform mat4 inverseProjectionWeapon;
uniform mat4 view;
uniform mat4 inverseView;
uniform vec3 viewPos;
uniform Light lights[16];
uniform int lightsCount;
uniform float screenWidth;
uniform float screenHeight;
uniform float time;
uniform int mode;
uniform float propogationGridSpacing;
uniform bool player1NeedsMuzzleFlash;
uniform bool player2NeedsMuzzleFlash;

uniform vec3 player1MuzzleFlashPosition;
uniform vec3 player2MuzzleFlashPosition;

const float PI = 3.14159265359;

//uniform int propogationTextureWidth;
//uniform int propogationTextureHeight;
//uniform int propogationTextureDepth;

//#define MAP_WIDTH   32
//#define MAP_HEIGHT  16
//#define MAP_DEPTH   50

////////////////////////
//                    //
//   Shadow mapping   //

vec3 gridSamplingDisk[20] = vec3[](
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

float ShadowCalculation(samplerCube depthTex, vec3 lightPos, vec3 fragPos, vec3 viewPos, vec3 Normal) {
    vec3 fragToLight = fragPos - lightPos;
    float currentDepth = length(fragToLight);
    float far_plane = 20.0; // you probably wanna send this as a uniform, or at least check it matches what you have in c++
    float shadow = 0.0;
    vec3 lightDir = fragPos - lightPos;
    float bias = max(0.0125 * (1.0 - dot(Normal, lightDir)), 0.00125);
    int samples = 20;
    float viewDistance = length(viewPos - fragPos);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;
    for(int i = 0; i < samples; ++i)     {
        float closestDepth = texture(depthTex, fragToLight + gridSamplingDisk[i] * diskRadius).r;
        closestDepth *= far_plane;  // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);
    return 1 - shadow;
}

/////////////
//         //
//   Fog   //

float getFogFactor(float d) {
    const float FogMax = 25.0;
    const float FogMin = 1.0;

    if (d>=FogMax) return 1;
    if (d<=FogMin) return 0;

    return 1 - (FogMax - d) / (FogMax - FogMin);
}

//////////////////////
//                  //
//   Tone mapping   //

vec3 filmic(vec3 x) {
  vec3 X = max(vec3(0.0), x - 0.004);
  vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, vec3(2.2));
}

float filmic(float x) {
  float X = max(0.0, x - 0.004);
  float result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
  return pow(result, 2.2);
}

vec3 Tonemap_ACES(const vec3 x) { // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

//////////////
//          //
//   Noise  //

vec3 filmPixel(vec2 uv) {
    mat2x3 uvs = mat2x3(uv.xxx, uv.yyy) + mat2x3(vec3(0, 0.1, 0.2), vec3(0, 0.3, 0.4));
    return fract(sin(uvs * vec2(12.9898, 78.233) * time) * 43758.5453);
}

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}


/////////////////////////
//                     //
//   Direct Lighting   //


float D_GGX(float NoH, float roughness) {
  float alpha = roughness * roughness;
  float alpha2 = alpha * alpha;
  float NoH2 = NoH * NoH;
  float b = (NoH2 * (alpha2 - 1.0) + 1.0);
  return alpha2 / (PI * b * b);
}

float G1_GGX_Schlick(float NdotV, float roughness) {
  //float r = roughness; // original
  float r = 0.5 + 0.5 * roughness; // Disney remapping
  float k = (r * r) / 2.0;
  float denom = NdotV * (1.0 - k) + k;
  return NdotV / denom;
}

float G_Smith(float NoV, float NoL, float roughness) {
  float g1_l = G1_GGX_Schlick(NoL, roughness);
  float g1_v = G1_GGX_Schlick(NoV, roughness);
  return g1_l * g1_v;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 microfacetBRDF(in vec3 L, in vec3 V, in vec3 N, in vec3 baseColor, in float metallicness, in float fresnelReflect, in float roughness) {
  vec3 H = normalize(V + L); // half vector
  // all required dot products
  float NoV = clamp(dot(N, V), 0.0, 1.0);
  float NoL = clamp(dot(N, L), 0.0, 1.0);
  float NoH = clamp(dot(N, H), 0.0, 1.0);
  float VoH = clamp(dot(V, H), 0.0, 1.0);       
  // F0 for dielectics in range [0.0, 0.16] 
  // default FO is (0.16 * 0.5^2) = 0.04
  vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
  // f0 = vec3(0.125);
  // in case of metals, baseColor contains F0
  f0 = mix(f0, baseColor, metallicness);
  // specular microfacet (cook-torrance) BRDF
  vec3 F = fresnelSchlick(VoH, f0);
  float D = D_GGX(NoH, roughness);
  float G = G_Smith(NoV, NoL, roughness);
  vec3 spec = (D * G * F) / max(4.0 * NoV * NoL, 0.001);  
  // diffuse
  vec3 notSpec = vec3(1.0) - F; // if not specular, use as diffuse
  notSpec *= 1.0 - metallicness; // no diffuse for metals
  vec3 diff = notSpec * baseColor / PI;   
  spec *= 1.05;
  vec3 result = diff + spec;
  return result;
}

vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic) {
	float fresnelReflect = 1.0; // 0.5 is what they used for box, 1.0 for demon
	vec3 viewDir = normalize(viewPos - WorldPos);    
	float lightRadiance = strength * 1;// * 1.25;
    vec3 lightDir = normalize(lightPos - WorldPos); 
//	float lightDist = length(lightPos - WorldPos);
//	float lightAttenuation = 1.0 / (lightDist*lightDist);

   float lightAttenuation = smoothstep(radius, 0, length(lightPos - WorldPos));
  //  lightAttenuation *= 0.25;
 //   float compression = 0.5;
   // lightAttenuation = pow(smoothstep(radius * 2, 0, length(lightPos - WorldPos)), compression);

//	lightAttenuation = clamp(lightAttenuation, 0.0, 0.9); // THIS IS WRONG, but does stop super bright region around light source and doesn't seem to affect anything else...
	float irradiance = max(dot(lightDir, Normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance;		
    //irradiance = clamp(irradiance, 0.0, 0.9);
	vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness);
    return brdf * irradiance * clamp(lightColor, 0, 1);
}

///////////////////////////
//                       //
//   Indirect Lighting   //

vec3 GetProbe(vec3 fragWorldPos, ivec3 offset, out float weight, vec3 Normal) {
    vec3 gridCoords = fragWorldPos / propogationGridSpacing;
    ivec3 base = ivec3(floor(gridCoords));
    vec3 a = gridCoords - base;
    vec3 probe_color = texelFetch(propgationGridTexture, base + offset, 0).rgb;
    vec3 probe_worldPos = (base + offset) * propogationGridSpacing;
    //vec3 v = normalize(probe_worldPos - fragWorldPos); // TODO: no need to normalize if only checking sign
    vec3 v = (probe_worldPos - fragWorldPos);
    float vdotn = dot(v, Normal);
    vec3 weights = mix(1. - a, a, offset);    
    if(vdotn > 0 && probe_color != vec3(0))
        weight = weights.x * weights.y * weights.z;
    else
        weight = 0.;
    return probe_color;
}

vec3 GetIndirectLighting(vec3 WorldPos, vec3 Normal) { // Interpolate visible probes      
    float w;
    vec3 light;
    float sumW = 0.;
    vec3 indirectLighting = vec3(0.);    
    light = GetProbe(WorldPos, ivec3(0, 0, 0), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 0, 1), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 1, 0), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 1, 1), w, Normal);
    indirectLighting += w * light;
    sumW += w;     
    light = GetProbe(WorldPos, ivec3(1, 0, 0), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 0, 1), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 1, 0), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 1, 1), w, Normal);
    indirectLighting += w * light;
    sumW += w;
    indirectLighting /= sumW;
    return indirectLighting;
}


void main() {

    // Sample GBuffer
    vec3 baseColor = texture(basecolorTexture, TexCoords).rgb;

//    baseColor *= 2.0;

    vec3 baseColor2 = baseColor;
    baseColor = pow(baseColor, vec3(2.2));
    vec3 normalMap =  texture2D(normalTexture, TexCoords).rgb;
    vec4 rma =  texture2D(rmaTexture, TexCoords);
    vec3 normal =  texture2D(normalTexture, TexCoords).rgb;

    // Get world position
    float projectionMatrixIndex = rma.a;
    mat4 inverseProjection = (projectionMatrixIndex == 0) ? inverseProjectionWeapon : inverseProjectionScene;
	float z = texture(depthTexture, TexCoords).x * 2.0f - 1.0f;
    vec2 clipSpaceTexCoord = TexCoords;
	vec4 clipSpacePosition = vec4(TexCoords * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePosition = inverseProjection * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;
    vec4 worldSpacePosition = inverseView * viewSpacePosition;    
    vec3 WorldPos = worldSpacePosition.xyz;

    // Get more stuff    
    float roughness = rma.r;
    float metallic = rma.g;
    float ao = rma.b;

  //  roughness = 0.99;
 //   metallic = 1;

    // Direct lighting
    vec3 directLighting = vec3(0);
    for(int i = 0; i < lightsCount; i++) {
        float shadow = ShadowCalculation(shadowMap[i], lights[i].position, WorldPos, viewPos, normal);
        vec3 ligthting = GetDirectLighting(lights[i].position, lights[i].color, lights[i].radius, lights[i].strength, normal, WorldPos, baseColor, roughness, metallic);
        directLighting += shadow * ligthting;
    }
    
   //. float
   vec3 muzzleFlashLightColor =  vec3(1, 0.7799999713897705, 0.5289999842643738);
   muzzleFlashLightColor *= 0.0025;
   float muzzleFlashRadius = 15;
   float muzzleFlashStrength = 10;

   
   vec3 directLightingFromPlayer1MuzzleFlashPosition = GetDirectLighting(player1MuzzleFlashPosition, muzzleFlashLightColor, muzzleFlashRadius, muzzleFlashStrength, normal, WorldPos, baseColor, roughness, metallic);
   float player1MuzzleFlashshadow =   ShadowCalculation(player1_shadowMap, player1MuzzleFlashPosition, WorldPos, viewPos, normal); 
   vec3 p1MuzzleFlashColor = directLightingFromPlayer1MuzzleFlashPosition * player1MuzzleFlashshadow;

   
   vec3 directLightingFromPlayer2MuzzleFlashPosition = GetDirectLighting(player2MuzzleFlashPosition, muzzleFlashLightColor, muzzleFlashRadius, muzzleFlashStrength, normal, WorldPos, baseColor, roughness, metallic);
   float player2MuzzleFlashshadow =   ShadowCalculation(player2_shadowMap, player2MuzzleFlashPosition, WorldPos, viewPos, normal); 
   vec3 p2MuzzleFlashColor = directLightingFromPlayer2MuzzleFlashPosition * player2MuzzleFlashshadow;

   

    directLighting = clamp(directLighting, 0, 1);

    
    if (player1NeedsMuzzleFlash) {
        directLighting += p1MuzzleFlashColor;
    }
    if (player2NeedsMuzzleFlash) {
        directLighting += p2MuzzleFlashColor;
    }

    // Indirect lighthing
    vec3 WorldPos2 = WorldPos + (normal * 0.01);
    vec3 indirectLighting = GetIndirectLighting(WorldPos2, normal);

    vec3 adjustedIndirectLighting = indirectLighting;
    float factor = min(1, roughness * 1.5);
    //factor = 1;
    //factor = min(1, roughness * 0.25) * 4;
    adjustedIndirectLighting *= (0.4) * vec3(factor); 
    adjustedIndirectLighting = max(adjustedIndirectLighting, vec3(0));
    adjustedIndirectLighting *= baseColor * 1.5;
  //  adjustedIndirectLighting *= 0.8;
    
   //  float contrast2 = 2.0;
	//adjustedIndirectLighting.rgb = adjustedIndirectLighting.rgb * contrast2;
   // adjustedIndirectLighting.rgb -= vec3(0.0025);

  //  adjustedIndirectLighting += sqrt(adjustedIndirectLighting) * 0.1;
    //adjustedIndirectLighting *= 0.85;

    // Composite
    vec3 composite = directLighting  + (adjustedIndirectLighting);
  //  vec3 composite = directLighting  + (indirectLighting * texture(basecolorTexture, TexCoords).rgb);
    FragColor.rgb = vec3(composite);

    // Final color
    if (mode == 0) {
        FragColor.rgb = composite;
        FragColor.a = 1;
    }
    if (mode == 1) {
        FragColor.rgba = vec4(0,0,0,0);
    }
    else if (mode == 2) {
        FragColor.rgb = directLighting;
        FragColor.a = 1;
    }
    else if (mode == 3) {
        FragColor.rgb = indirectLighting ;
        FragColor.rgb = adjustedIndirectLighting;
        FragColor.a = 1;
    }


    
        float d = distance(viewPos, WorldPos);
        float alpha = getFogFactor(d);
        vec3 FogColor = vec3(0.0);
        FragColor.rgb = mix(FragColor.rgb, FogColor, alpha);
	    FragColor.rgb = pow(FragColor.rgb, vec3(1.0/2.2)); 
        FragColor.rgb = mix(FragColor.rgb, Tonemap_ACES(FragColor.rgb), 1.0);
        FragColor.rgb = mix(FragColor.rgb, Tonemap_ACES(FragColor.rgb), 0.25);

           // Noise
        vec2 uv = gl_FragCoord.xy / vec2(screenWidth, screenHeight);
        vec2 filmRes = vec2(screenWidth, screenHeight);
        vec2 coord = gl_FragCoord.xy;
        vec2 rest = modf(uv * filmRes, coord);
        vec3 noise00 = filmPixel(coord / filmRes);
        vec3 noise01 = filmPixel((coord + vec2(0, 1)) / filmRes);
        vec3 noise10 = filmPixel((coord + vec2(1, 0)) / filmRes);
        vec3 noise11 = filmPixel((coord + vec2(1, 1)) / filmRes);
        vec3 noise = mix(mix(noise00, noise01, rest.y), mix(noise10, noise11, rest.y), rest.x) * vec3(0.7, 0.6, 0.8);
        float noiseSpeed = 30.0;
        float x = rand(uv + rand(vec2(int(time * noiseSpeed), int(-time * noiseSpeed))));
        float noiseFactor = 0.049;
        FragColor.rgb = FragColor.rgb + (x * -noiseFactor) + (noiseFactor / 2);


        
        // Vignette         
        uv = gl_FragCoord.xy / vec2(screenWidth * 1, screenHeight * 1);
        uv *=  1.0 - uv.yx;           
        float vig = uv.x*uv.y * 15.0; // multiply with sth for intensity    
        vig = pow(vig, 0.05); // change pow for modifying the extend of the  vignette    
        FragColor.rgb *= vec3(vig);

        // Contrast
	    float contrast = 1.5;
        vec3 finalColor = FragColor.rgb;
	    FragColor.rgb = FragColor.rgb * contrast;

        // Brightness
        FragColor.rgb -= vec3(0.085);


    //FragColor.rgb = vec3(0);

    // not used bruh
    /*
    for(int i = 0; i < lightsCount; i++)
    {
        vec3 lightDir = normalize(lights[i].position - WorldPos);     
	    float lightDist = length(lights[i].position - WorldPos);
	    float lightAttenuation = 1.0 / (lightDist*lightDist);
	    float ndotl = dot(lightDir, normal);
    }
    */
    //FragColor.rgb = vec3(baseColor);
  // FragColor.rgb = vec3(baseColor);
    //FragColor.rgb = pow(FragColor.rgb, vec3(1.0/2.2)); 
 //   FragColor.rgb = baseColor;

    
 //  FragColor.rgb = vec3(baseColor.rgb);


 /*   if (gl_FragCoord.y < 50) {
      FragColor.rgb = vec3(0,0,0);
    }
    if (gl_FragCoord.y > (432 * 2) - 50) {
         FragColor.rgb = vec3(0,0,0);
    }
    */

   // FragColor.rgb = normal;
 //  float originalZ = gl_FragCoord.z / gl_FragCoord.w;
 // FragColor.rgb = vec3(originalZ);
}