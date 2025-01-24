#version 420 core

out vec4 FragColor;
in vec2 TexCoord;
in vec3 WorldPos;
in vec3 Normal;
uniform vec3 color;
uniform vec3 viewPos;
uniform int textureFlag;

layout (binding = 0) uniform sampler2D basecolorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;
layout (binding = 5) uniform samplerCube shadowMap[16];

in vec3 attrNormal;
in vec3 attrTangent;
in vec3 attrBiTangent;

const float PI = 3.14159265359;

struct Light {
	vec3 position;
	vec3 color;
	float strength;
	float radius;
};

uniform Light lights[16];
uniform int lightsCount;

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
   float lightAttenuation = smoothstep(radius, 0, length(lightPos - WorldPos));	float irradiance = max(dot(lightDir, Normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance;		
	vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness);
    return brdf * irradiance * clamp(lightColor, 0, 1);
}

vec3 Tonemap_ACES(const vec3 x) { // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}


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

void main() {

    vec3 baseColor = texture(basecolorTexture, TexCoord).rgb;
    vec3 normalMap = texture2D(normalTexture, TexCoord).rgb;
    mat3 tbn = mat3(normalize(attrTangent), normalize(attrBiTangent), normalize(attrNormal));	
    vec3 normal = normalize(tbn * (normalMap.rgb * 2.0 - 1.0));

    //vec3 rma =  texture2D(rmaTexture, TexCoord).rgb;
   //	mat3 tbn = mat3(normalize(attrTangent), normalize(attrBiTangent), normalize(attrNormal));	
	//vec3 normal = normalize(tbn * (normalMap.rgb * 2.0 - 1.0));

    
     // Sample GBuffer

//    baseColor *= 2.0;

    baseColor = pow(baseColor, vec3(2.2));
    vec4 rma =  texture2D(rmaTexture, TexCoord);
  //  vec3 normal =  texture2D(normalTexture, TexCoord).rgb;
    float roughness = rma.r;
    float metallic = rma.g;

	    // Direct lighting
    vec3 directLighting = vec3(0);
    for(int i = 0; i < lightsCount; i++) {
        float shadow = ShadowCalculation(shadowMap[i], lights[i].position, WorldPos, viewPos, normal);
        vec3 lighting = GetDirectLighting(lights[i].position, lights[i].color, lights[i].radius, lights[i].strength, normal, WorldPos, baseColor, roughness, metallic);
        directLighting += shadow * lighting;
        //directLighting += ligthting;
    }

    FragColor.rgb = directLighting.rgb;
    

    // gamma correction
    FragColor.rgb = pow(FragColor.rgb, vec3(1.0/2.2)); 

    // tone map
    //FragColor.rgb = mix(FragColor.rgb, Tonemap_ACES(FragColor.rgb), 1.0);
    //FragColor.rgb = mix(FragColor.rgb, Tonemap_ACES(FragColor.rgb), 0.25);

}

