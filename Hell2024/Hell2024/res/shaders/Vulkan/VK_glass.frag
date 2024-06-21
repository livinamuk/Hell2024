#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in flat int BaseColorTextureIndex;
layout (location = 3) in flat int NormalTextureIndex;
layout (location = 4) in flat int RMATextureIndex;
layout (location = 5) in vec3 attrNormal;
layout (location = 6) in vec3 attrTangent;
layout (location = 7) in vec3 attrBiTangent;
layout (location = 8) in vec3 WorldPos;
layout (location = 9) in flat int playerIndex;
layout (location = 10) in vec3 ViewPos;


layout (location = 0) out vec4 glassColorOut;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[1024];



struct Light {
	float posX;
	float posY;
	float posZ;
	float colorR;
	float colorG;
	float colorB;
	float strength;
	float radius;
};

layout(set = 0, binding = 4) readonly buffer B {Light data[];} lights;




const int samples = 35,
          LOD = 2,         // gaussian done on MIPmap at scale LOD
          sLOD = 1 << LOD; // tile size = 2^LOD
const float sigma = float(samples) * .25;

float gaussian(vec2 i) {
    return exp( -.5* dot(i/=sigma,i) ) / ( 6.28 * sigma*sigma );
}

vec4 blur(sampler2D sp, vec2 U, vec2 scale) {
    vec4 O = vec4(0);
    int s = samples/sLOD;
    for ( int i = 0; i < s*s; i++ ) {
        vec2 d = vec2(i%s, i/s)*float(sLOD) - float(samples)/2.;
        O += gaussian(d) * textureLod( sp, U + scale * d , float(LOD) );
    }
    return O / O.a;
}


const float PI = 3.14159265359;



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
	vec3 viewDir = normalize(ViewPos - WorldPos);
	float lightRadiance = strength * 1;// * 1.25;
    vec3 lightDir = normalize(lightPos - WorldPos);
    float lightAttenuation = smoothstep(radius, 0, length(lightPos - WorldPos));	float irradiance = max(dot(lightDir, Normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance;
	vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness);
    return brdf * irradiance * clamp(lightColor, 0, 1);
}


void main() {

    vec4 baseColor = texture(sampler2D(textures[BaseColorTextureIndex], samp), texCoord);
    vec3 normalMap = texture(sampler2D(textures[NormalTextureIndex], samp), texCoord).rgb;
    vec3 rma = texture(sampler2D(textures[RMATextureIndex], samp), texCoord).rgb;
	mat3 tbn = mat3(normalize(attrTangent), normalize(attrBiTangent), normalize(attrNormal));
	vec3 normal = normalize(tbn * (normalMap.rgb * 2.0 - 1.0));



	float roughness = rma.r;
    float metallic = 0;


    vec3 color = vec3(0);
	for (int i = 0; i < 8; i++) {
       vec3 lightPos = vec3(lights.data[i].posX, lights.data[i].posY, lights.data[i].posZ);
       vec3 lightColor = vec3(lights.data[i].colorR, lights.data[i].colorG, lights.data[i].colorB);
       float radius = lights.data[i].radius;
       float strength = lights.data[i].strength;
	          vec3 directLight = GetDirectLighting(lightPos, lightColor, radius, strength, normal, WorldPos, baseColor.rgb, roughness, metallic);
	   directLight.rgb = pow(directLight.rgb, vec3(1.0/2.2));
       color += directLight;
	}

	 vec3 lightPos = vec3(lights.data[0].posX, lights.data[0].posY, lights.data[0].posZ);


	glassColorOut.rgb = color;// vec3(0,1,0);
//	glassColorOut.rgb = vec3(1,0,0);



}