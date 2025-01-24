#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) out vec4 SpecularColorOut;
layout (location = 1) out vec4 DirectLightingColorOut;

in vec3 Color;
in vec2 TexCoords;
in vec3 WorldPos;

uniform bool useUniformColor;
uniform vec3 uniformColor;
uniform vec3 viewPos;
uniform int playerIndex;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 inverseView;
uniform vec3 cameraForward;

in flat int BaseColorTextureIndex;
in flat int NormalTextureIndex;
in flat int RMATextureIndex;

in vec3 attrNormal;
in vec3 attrTangent;
in vec3 attrBiTangent;

readonly restrict layout(std430, binding = 0) buffer textureSamplerers {
	uvec2 textureSamplers[];
};



/////////////////////////
//                     //
//   Direct Lighting   //


const float PI = 3.14159265359;

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

vec3 microfacetBRDF(in vec3 L, in vec3 V, in vec3 N, in vec3 baseColor, in float metallicness, in float fresnelReflect, in float roughness, in vec3 WorldPos) {
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
 // spec *= 1.05;
  vec3 result = diff ;//+ spec;

  return result;
}



vec3 microfacetBRDFSpecularOnly(in vec3 L, in vec3 V, in vec3 N, in vec3 baseColor, in float metallicness, in float fresnelReflect, in float roughness, in vec3 WorldPos) {
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
  vec3 result = spec;

  return result;
}


struct Light {
    float posX;
    float posY;
    float posZ;
    float colorR;
    float colorG;
    float colorB;
    float strength;
    float radius;
    int shadowMapIndex;
    int contributesToGI;
    float padding0;
    float padding1;
};


layout(std430, binding = 2) readonly buffer Lights {
    Light lights[];
};


vec3 GetDirectLighting(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
	float fresnelReflect = 1.0; // 0.5 is what they used for box, 1.0 for demon

	vec3 viewDir = normalize(viewPos - WorldPos);
	float lightRadiance = strength * 1;// * 1.25;
	vec3 lightDir = normalize(lightPos - WorldPos);
	float lightAttenuation = smoothstep(radius, 0, length(lightPos - WorldPos));
	// lightAttenuation = clamp(lightAttenuation, 0.0, 0.9); // THIS IS WRONG, but does stop super bright region around light source and doesn't seem to affect anything else...
	float irradiance = max(dot(lightDir, Normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance;
	vec3 brdf = microfacetBRDF(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness, WorldPos);
	return brdf * irradiance * clamp(lightColor, 0, 1);
}


vec3 GetSpecularOnly(vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
	float fresnelReflect = 1.0; // 0.5 is what they used for box, 1.0 for demon

	vec3 viewDir = normalize(viewPos - WorldPos);
	float lightRadiance = strength * 1;// * 1.25;
	vec3 lightDir = normalize(lightPos - WorldPos);
	float lightAttenuation = smoothstep(radius, 0, length(lightPos - WorldPos));
	// lightAttenuation = clamp(lightAttenuation, 0.0, 0.9); // THIS IS WRONG, but does stop super bright region around light source and doesn't seem to affect anything else...
	float irradiance = max(dot(lightDir, Normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance;
	vec3 brdf = microfacetBRDFSpecularOnly(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness, WorldPos);
	return brdf * irradiance * clamp(lightColor, 0, 1);
}


vec3 GetDirectionalSpecularOnly(vec3 lightDir, vec3 lightPos, vec3 lightColor, float radius, float strength, vec3 Normal, vec3 WorldPos, vec3 baseColor, float roughness, float metallic, vec3 viewPos) {
	float fresnelReflect = 1.0;
	vec3 viewDir = normalize(viewPos - WorldPos);
	float lightRadiance = strength;
	float lightAttenuation = smoothstep(radius, 0, length(lightPos - WorldPos));
	float irradiance = max(dot(lightDir, Normal), 0.0) ;
	irradiance *= lightAttenuation * lightRadiance;
	vec3 brdf = microfacetBRDFSpecularOnly(lightDir, viewDir, Normal, baseColor, metallic, fresnelReflect, roughness, WorldPos);
	return brdf * irradiance * clamp(lightColor, 0, 1);
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






vec3 GetSpotLighting(vec3 fragPos, vec3 normal, vec3 viewPos, vec3 spotLightDirection) {
    vec3 position = viewPos;
    vec3 direction = spotLightDirection;
	float cutoff = cos(radians(10.5));
    float outerCutoff = cos(radians(47.5));
    float constant = 0.25;
    float linear = 0.09;
    float quadratic = 0.042;
    vec3 ambient = vec3(0.2);
    vec3 diffuse = vec3(0.8);
    vec3 specular = vec3(1.0);

    vec3 ambientLight = ambient;
    vec3 lightDir = normalize(position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuseLight = diffuse * diff;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); // Shininess factor
    vec3 specularLight = specular * spec;
    float theta = dot(lightDir, -direction);
    float epsilon = cutoff - outerCutoff;
    float intensity = clamp((theta - outerCutoff) / epsilon, 0.0, 1.0);
    float distance = length(position - fragPos);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));
    vec3 result = (ambientLight + intensity * (diffuseLight + specularLight)) * attenuation;

    return result;
}




void main() {


    vec4 baseColor = texture(sampler2D(textureSamplers[BaseColorTextureIndex]), TexCoords);
    vec4 normalMap = texture(sampler2D(textureSamplers[NormalTextureIndex]), TexCoords);
    vec4 rma = texture(sampler2D(textureSamplers[RMATextureIndex]), TexCoords);

	float roughness = rma.r;
	float metallic = rma.g;




	mat3 tbn = mat3(normalize(attrTangent), normalize(attrBiTangent), normalize(attrNormal));
	vec3 normal = normalize(tbn * (normalMap.rgb * 2.0 - 1.0));

	vec3 directLighting = vec3(0);

	for (int i = 0; i < 8; i++) {
		Light light = lights[i];
		vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
		vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
		float lightRadius = light.radius;
		float lightStrength = light.strength;
		const vec3 lightDirection = normalize(lightPosition - WorldPos);
		vec3 ligthting = GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal, WorldPos, baseColor.rgb, roughness, metallic, viewPos);
		//float shadow = ShadowCalculation(shadowMap[i], lightPosition, WorldPos, viewPos, normal);
		float shadow = 1;
		directLighting += ligthting * vec3(shadow);
	}


	directLighting = directLighting;
	directLighting = mix(directLighting, Tonemap_ACES(directLighting), 1.0);
	directLighting = pow(directLighting, vec3(1.0/2.2));
	directLighting = mix(directLighting, Tonemap_ACES(directLighting), 0.1235);
	directLighting = mix(directLighting, Tonemap_ACES(directLighting), 0.95);

	float contrast = 1.15;
	directLighting.rgb = directLighting.rgb * contrast;
	directLighting.rgb -= vec3(0.020);

	DirectLightingColorOut.rgb = directLighting;
	DirectLightingColorOut.a = 1.0;



	vec3 specularLighting = vec3(0);

	for (int i = 0; i < 8; i++) {
		Light light = lights[i];
		vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
		vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);
		float lightRadius = light.radius;
		float lightStrength = light.strength;
		const vec3 lightDirection = normalize(lightPosition - WorldPos);
		vec3 ligthting = GetSpecularOnly(lightPosition, lightColor, lightRadius, lightStrength, normal, WorldPos, baseColor.rgb, roughness, metallic, viewPos);
		float shadow = 1;
		specularLighting += ligthting * vec3(shadow);
	}


	specularLighting = mix(specularLighting, Tonemap_ACES(specularLighting), 1.0);
	specularLighting = pow(specularLighting, vec3(1.0/2.2));
	specularLighting = mix(specularLighting, Tonemap_ACES(specularLighting), 0.1235);
	specularLighting = mix(specularLighting, Tonemap_ACES(specularLighting), 0.95);

	SpecularColorOut.rgb = specularLighting;





	SpecularColorOut.rgb = vec3(0);

	// player flashlights
	for (int i = 0; i < 2; i++) {	
		vec3 spotLightPos = viewPos;
		if (playerIndex != i) {
			spotLightPos += (cameraForward * 0.125);
		}		
		spotLightPos -= vec3(0, 0.0, 0);
		vec3 dir = normalize(spotLightPos - (viewPos - cameraForward));
		vec3 spotLightingFactor = GetSpotLighting(WorldPos, normalize(normal), spotLightPos, dir);	
		vec3 spotLightColor = vec3(1, 0.8799999713897705, 0.6289999842643738);


		
		//vec3 spotLightColor = vec3(1, 0.8799999713897705, 0.6289999842643738);
		//-vec3 spotLighting = GetDirectLighting(spotLightPos, spotLightColor, 100, 1.0, normal, worldSpacePosition.xyz, baseColor, roughness, metallic, camPos);
		
		vec3 spotLighting = GetDirectionalSpecularOnly(viewPos, cameraForward, spotLightColor, 100, 1.0, normal, WorldPos, baseColor.rgb, roughness, metallic, viewPos);		
		SpecularColorOut.rgb += spotLighting;
		
		spotLighting = GetDirectionalSpecularOnly(viewPos, -cameraForward, spotLightColor, 100, 1.0, -normal, WorldPos, baseColor.rgb, roughness, metallic, viewPos);	
		SpecularColorOut.rgb += spotLighting;

	}




	SpecularColorOut.a = 1.0;

  // FragOut.rgb = vec3(1, 0, 0);
}
