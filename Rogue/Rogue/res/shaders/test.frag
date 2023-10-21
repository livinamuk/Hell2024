#version 420 core

//layout (binding = 0) uniform sampler2D tex;
layout (binding = 0) uniform sampler3D tex;
layout (binding = 1) uniform samplerCube shadowMap0;
layout (binding = 2) uniform samplerCube shadowMap1;
layout (binding = 3) uniform samplerCube shadowMap2;
layout (binding = 4) uniform samplerCube shadowMap3;
layout (binding = 5) uniform sampler2D basecolorTexture;
layout (binding = 6) uniform sampler2D normalTexture;
layout (binding = 7) uniform sampler2D rmaTexture;

uniform vec3 lightPosition[4];
uniform vec3 lightColor[4];
uniform float lightStrength[4];
uniform float lightRadius[4];

in vec2 TexCoord;
in vec3 WorldPos;

out vec4 FragColor;
uniform vec3 viewPos;
uniform vec3 camForward;
uniform vec3 lightingColor;
uniform vec3 indirectLightingColor;
uniform int mode;

uniform mat4 view;

#define MAP_WIDTH   32
#define MAP_HEIGHT  22
#define MAP_DEPTH   36
#define VOXEL_SIZE  0.2
#define PROPOGATION_SPACING 1.0
#define PROBE_SPACING  VOXEL_SIZE * PROPOGATION_SPACING

in vec3 attrNormal;
in vec3 attrTangent;
in vec3 attrBiTangent;

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

vec3 Tonemap_ACES(const vec3 x) {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

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
  

float ShadowCalculation(samplerCube depthTex, vec3 lightPos, vec3 fragPos, vec3 viewPos, vec3 Normal)
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



const float PI = 3.14159265359;
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal) {
    float dist = distance(WorldPos, lightPos);
    //float att = 1.0 / (1.0 + 0.1 * dist + 0.01 * dist * dist);
    float att = smoothstep(radius, 0.0, length(lightPos - WorldPos));
    vec3 n = normalize(Normal);
    vec3 l = normalize(lightPos - WorldPos);
    float ndotl = clamp(dot(n, l), 0.0f, 1.0f);
    
    
    vec3 radiance = vec3(lightColor) * att * strength * ndotl * 1.0;



    // Texturing
    vec3 baseColor =  pow(texture(basecolorTexture, TexCoord).rgb, vec3(2.2));
    vec3 normalMap =  texture2D(normalTexture, TexCoord).rgb;
    vec3 rma =  texture2D(rmaTexture, TexCoord).rgb;
    vec3 albedo = baseColor;
    float roughness = rma.r;
    float metallic = rma.g;
    float ao = 1;//rma.b;

   	mat3 tbn = mat3(normalize(attrTangent), normalize(attrBiTangent), normalize(attrNormal));	
	vec3 normal = (tbn * normalize(normalMap.rgb * 2.0 - 1.0));
	normal =  normalize(normal.rgb);

    vec3 N = normal;
    vec3 V = normalize(viewPos - WorldPos);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

        vec3 L = normalize(lightPos - WorldPos);
        vec3 H = normalize(V + L);

            // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
           
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	  

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        return (kD * albedo / PI + specular) * radiance * NdotL; 
}

vec3 GetProbe(vec3 fragWorldPos, ivec3 offset, out float weight, vec3 Normal) {

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

vec3 microfacetBRDF(in vec3 L, in vec3 V, in vec3 N, in vec3 baseColor, in float metallicness, in float fresnelReflect, in float roughness, out vec3 specularContribution) {
     
  vec3 H = normalize(V + L); // half vector

  // all required dot products
  float NoV = clamp(dot(N, V), 0.0, 1.0);
  float NoL = clamp(dot(N, L), 0.0, 1.0);
  float NoH = clamp(dot(N, H), 0.0, 1.0);
  float VoH = clamp(dot(V, H), 0.0, 1.0);     
  
  // F0 for dielectics in range [0.0, 0.16] 
  // default FO is (0.16 * 0.5^2) = 0.04
  vec3 f0 = vec3(0.16 * (fresnelReflect * fresnelReflect)); 
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
  
  specularContribution = spec;
  return diff + spec;
}


vec3 GetDirectLighting2(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal) {
//vec3 CalculatePBR (vec3 baseColor, vec3 normal, float roughness, float metallic, float ao, vec3 worldPos, vec3 camPos, Light light, int materialType) {
	
	// compute direct light	  
	float fresnelReflect = 1.0;											// this is what they used for box, 1.0 for demon
	vec3 viewDir = normalize(viewPos - WorldPos);    
	float lightRadiance = strength;
    vec3 lightDir = normalize(lightPos - WorldPos);           // they use something more sophisticated with a sphere
	float lightDist = max(length(lightPos - WorldPos), 0.1);
	float lightAttenuation = 1.0 / (lightDist*lightDist);
	//lightAttenuation = clamp(lightAttenuation, 0.0, 1.0);					// THIS IS WRONG, but does stop super bright region around light source and doesn't seem to affect anything else...
	float irradiance = max(dot(lightDir, Normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance ;
		
	vec3 radiance = vec3(0.0);
	vec3 specularContribution = vec3(0);

    // Texturing
    vec3 baseColor =  pow(texture(basecolorTexture, TexCoord).rgb, vec3(2.2));
   //  baseColor =  texture(basecolorTexture, TexCoord).rgb;
    vec3 normalMap =  texture2D(normalTexture, TexCoord).rgb;
    vec3 rma =  texture2D(rmaTexture, TexCoord).rgb;
    vec3 albedo = baseColor;
    float roughness = rma.r;
    float metallic = rma.g;
    float ao = rma.b;

   	//mat3 tbn = mat3(normalize(attrTangent), normalize(attrBiTangent), normalize(attrNormal));	
	//vec3 normal = (tbn * normalize(normalMap.rgb * 2.0 - 1.0));
	//normal =  normalize(normal.rgb);

	// if receives light
	if(irradiance > 0.0) { 
		vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness, specularContribution);
		radiance += brdf * irradiance * lightColor; // diffuse shading
	}
	
	return radiance;
}



void main()
{
    // Texturing
    vec3 baseColor =  pow(texture(basecolorTexture, TexCoord).rgb, vec3(2.2));
    baseColor =  texture(basecolorTexture, TexCoord).rgb;
    vec3 normalMap =  texture2D(normalTexture, TexCoord).rgb;
    //vec3 rma =  texture2D(rmaTexture, TexCoord).rgb;
    //vec3 albedo = baseColor;
    //float roughness = rma.r;
    //float metallic = rma.g;
    //float ao = rma.b;
   	mat3 tbn = mat3(normalize(attrTangent), normalize(attrBiTangent), normalize(attrNormal));	
	vec3 normal = (tbn * normalize(normalMap.rgb * 2.0 - 1.0));
	normal =  normalize(normal.rgb);
    
 


    float doom = calculateDoomFactor(WorldPos, viewPos, 4, 1);
    doom = getFogFactor(WorldPos, viewPos);
    
    // uniform color
    //FragColor = vec4(Normal * 0.5 + 0.5, 1.0f);
    //FragColor = vec4(baseColor * lightingColor, 1.0f);

    float worldSpaceMapWidth = MAP_WIDTH * VOXEL_SIZE;
    float worldSpaceMapHeight = MAP_HEIGHT * VOXEL_SIZE;
    float worldSpaceMapDepth = MAP_DEPTH * VOXEL_SIZE;

    vec3 propogrationGridOffset = vec3(-0.1);//VOXEL_SIZE * PROPOGATION_SPACING ;/// 2.0;

    vec3 WorldPos2 = WorldPos + attrNormal * 0.01;
    vec3 texCoords = (WorldPos2 - propogrationGridOffset) / vec3(worldSpaceMapWidth, worldSpaceMapHeight, worldSpaceMapDepth);
       
    // Interpolate visible probes
    float w;
    vec3 light;
    float sumW = 0.;
    vec3 indirectLighting = vec3(0.);
    
    light = GetProbe(WorldPos, ivec3(0, 0, 0), w, attrNormal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 0, 1), w, attrNormal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 1, 0), w, attrNormal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(0, 1, 1), w, attrNormal);
    indirectLighting += w * light;
    sumW += w;
     
    light = GetProbe(WorldPos, ivec3(1, 0, 0), w, attrNormal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 0, 1), w, attrNormal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 1, 0), w, attrNormal);
    indirectLighting += w * light;
    sumW += w;
    light = GetProbe(WorldPos, ivec3(1, 1, 1), w, attrNormal);
    indirectLighting += w * light;
    sumW += w;

    indirectLighting /= sumW;

    // Direct lightihng
    float shadow0 = ShadowCalculation(shadowMap0, lightPosition[0], WorldPos, viewPos, normal);
    vec3 ligthting0 = GetDirectLighting(lightPosition[0], lightColor[0], lightRadius[0], lightStrength[0], normal);
    float shadow1 = ShadowCalculation(shadowMap1, lightPosition[1], WorldPos, viewPos, normal);
    vec3 ligthting1 = GetDirectLighting(lightPosition[1], lightColor[1], lightRadius[1], lightStrength[1], normal);
    float shadow2 = ShadowCalculation(shadowMap2, lightPosition[2], WorldPos, viewPos, normal);
    vec3 ligthting2 = GetDirectLighting(lightPosition[2], lightColor[2], lightRadius[2], lightStrength[2], normal);
    float shadow3 = ShadowCalculation(shadowMap3, lightPosition[3], WorldPos, viewPos, normal);
    vec3 ligthting3 = GetDirectLighting(lightPosition[3], lightColor[3], lightRadius[3], lightStrength[3], normal);     
    vec3 directLighting = vec3(0);
    
    //shadow0 = 1;
    //shadow1 = 1;
    //shadow2 = 1;
    //shadow3 = 1;

    directLighting += shadow0 * ligthting0;
    directLighting += shadow1 * ligthting1;
    directLighting += shadow2 * ligthting2;
    directLighting += shadow3 * ligthting3;
        
    indirectLighting *= (0.1); 

    // Composite
    vec3 composite = directLighting + indirectLighting * 1;
    


    // Fog
    //vec3 fogColor = vec3(0.050);
    //composite = mix(composite, fogColor, doom);
    
    // Final color
    if (mode == 0) {
        FragColor.rgb = directLighting + (indirectLighting * baseColor);
      //  FragColor.rgb = directLighting ;
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
        FragColor.rgb = indirectLighting;
        FragColor.a = 1;
    }
    
    // Tonemap
	FragColor.rgb = pow(FragColor.rgb, vec3(1.0/2.2)); 
	FragColor.rgb = Tonemap_ACES(FragColor.rgb);
    
    	// Filmic tonemapping
//	FragColor.rgb = mix(FragColor.rgb, filmic(FragColor.rgb), 0.5);

    //    FragColor.rgb = normal;
}
