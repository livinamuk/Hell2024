#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct RayPayload {
	vec4 color;
	int lightIndex;
	float viewPosX;
	float viewPosY;
	float viewPosZ;
};

struct RenderItem3D {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    int meshIndex;
    int baseColorTextureIndex;
    int normalMapTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int castShadow;
    int useEmissiveMask;
    float emissiveColorR;
    float emissiveColorG;
    float emissiveColorB;
    float aabbMinX;
    float aabbMinY;
    float aabbMinZ;
    float aabbMaxX;
    float aabbMaxY;
    float aabbMaxZ;
    float padding0;
    float padding1;
    float padding2;
};

struct Vertex {
	vec3 position;
    vec3 normal;
    vec2 texCoord;
    vec3 tangent;
};

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

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 1) rayPayloadEXT bool isShadowed;

layout(set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;
layout(std140,set = 0, binding = 9) readonly buffer A {RenderItem3D data[];} tlasInstances;
layout(set = 0, binding = 4, scalar) readonly buffer B {Light data[];} lights;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[1024];

layout(set = 3, binding = 0, scalar) readonly buffer Vertices { Vertex v[]; } vertices;
layout(set = 3, binding = 1) readonly buffer Indices { uint i[]; } indices;

vec3 mixBary(vec3 a, vec3 b, vec3 c, vec3 barycentrics) {
	return (a * barycentrics.x + b * barycentrics.y + c * barycentrics.z);
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
  spec *= 1.05;
  vec3 result = diff + spec;

  float test = (notSpec.x + notSpec.y + notSpec.z) * 0.33;
  
  test = 1 - test;
  test *= (1 - roughness);
  test *= metallicness;
  //result = vec3(test * test);

  float test2 = (spec.x + spec.y + spec.z) * 0.33;
 // result = vec3(spec);

  return result ;//* vec3(test);
}

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






vec3 Tonemap_ACES(const vec3 x) { // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}








void main() {

// Get vertex data from hit
	RenderItem3D renderItem = tlasInstances.data[gl_InstanceID]; // was gl_InstanceID
	mat4 modelMatrix = renderItem.modelMatrix;
	int vertexOffset = renderItem.vertexOffset;
	int indexOffset = renderItem.indexOffset;
	int baseColorTextureIndex = renderItem.baseColorTextureIndex;
	int normalTextureIndex = renderItem.normalMapTextureIndex;
	int rmaTextureIndex = renderItem.rmaTextureIndex;
    uint index0 = indices.i[3 * gl_PrimitiveID + indexOffset];
    uint index1 = indices.i[3 * gl_PrimitiveID + 1 + indexOffset];
    uint index2 = indices.i[3 * gl_PrimitiveID + 2 + indexOffset];
    Vertex v0 = vertices.v[index0 + vertexOffset];
    Vertex v1 = vertices.v[index1 + vertexOffset];
    Vertex v2 = vertices.v[index2 + vertexOffset];	
	const vec3 pos0 = v0.position.xyz;
	const vec3 pos1 = v1.position.xyz;
	const vec3 pos2 = v2.position.xyz;
	const vec3 nrm0 = v0.normal.xyz;
	const vec3 nrm1 = v1.normal.xyz;
	const vec3 nrm2 = v2.normal.xyz;
	const vec2 uv0  = v0.texCoord;
	const vec2 uv1  = v1.texCoord;
	const vec2 uv2  = v2.texCoord;
	const vec4 tng0 = vec4(v0.tangent, 0);
	const vec4 tng1 = vec4(v1.tangent, 0);
	const vec4 tng2 = vec4(v2.tangent, 0);
		
	// Texture co-ordinate
	const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    const vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;

	// World normal
	const vec3 vnormal = normalize(mixBary(nrm0, nrm1, nrm2, barycentrics));
	vec3 normal    = normalize(vec3(vnormal * gl_WorldToObjectEXT));
	vec3 tangent = normalize(vec3(normalize(mixBary(tng0.xyz, tng1.xyz, tng2.xyz, barycentrics)) * gl_WorldToObjectEXT));
	vec3 bitangent = cross(normal, tangent);
	
	const vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
	
    vec4 baseColor = texture(sampler2D(textures[baseColorTextureIndex], samp), texCoord).rgba;
 //   vec4 normal2 = texture(sampler2D(textures[normalTextureIndex], samp), texCoord).rgba;
    vec3 rma = texture(sampler2D(textures[rmaTextureIndex], samp), texCoord).rgb;
	
	float roughness = rma.r;
	float metallic = rma.g;
	

		Light light = lights.data[rayPayload.lightIndex];		
		vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
		vec3 lightColor = vec3(light.colorR, light.colorG, light.colorB);

		vec3 viewPos = vec3(rayPayload.viewPosX, rayPayload.viewPosY, rayPayload.viewPosZ);

	vec3 directLighting = GetDirectLighting(lightPosition, lightColor, light.radius, light.strength, normal, worldPos, baseColor.rgb, roughness, metallic, viewPos);
	
	//directLighting.rgb = mix(directLighting.rgb, Tonemap_ACES(directLighting.rgb), 1.0);
	//directLighting.rgb = mix(directLighting.rgb, Tonemap_ACES(directLighting.rgb), 0.35);	
	//directLighting.rgb = pow(directLighting.rgb, vec3(1.0/2.2)); 


	rayPayload.color = vec4(directLighting.rgb, 1);

//	rayPayload.color = vec4(normal, 1.0);

}

/*
void main() {

	// Get vertex data from hit
	RenderItem3D renderItem = tlasInstances.data[gl_InstanceCustomIndexEXT]; // was gl_InstanceID
	mat4 modelMatrix = renderItem.modelMatrix;
	int vertexOffset = r
	enderItem.vertexOffset;
	int indexOffset = renderItem.indexOffset;
    uint index0 = indices.i[3 * gl_PrimitiveID + indexOffset];
    uint index1 = indices.i[3 * gl_PrimitiveID + 1 + indexOffset];
    uint index2 = indices.i[3 * gl_PrimitiveID + 2 + indexOffset];
    Vertex v0 = vertices.v[index0 + vertexOffset];
    Vertex v1 = vertices.v[index1 + vertexOffset];
    Vertex v2 = vertices.v[index2 + vertexOffset];	
	const vec3 pos0 = v0.position.xyz;
	const vec3 pos1 = v1.position.xyz;
	const vec3 pos2 = v2.position.xyz;
	const vec3 nrm0 = v0.normal.xyz;
	const vec3 nrm1 = v1.normal.xyz;
	const vec3 nrm2 = v2.normal.xyz;
	const vec2 uv0  = v0.texCoord;
	const vec2 uv1  = v1.texCoord;
	const vec2 uv2  = v2.texCoord;
	const vec4 tng0 = vec4(v0.tangent, 0);
	const vec4 tng1 = vec4(v1.tangent, 0);
	const vec4 tng2 = vec4(v2.tangent, 0);
		
	// Texture co-ordinate
	const vec3 barycentrics = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    const vec2 texCoord = v0.texCoord * barycentrics.x + v1.texCoord * barycentrics.y + v2.texCoord * barycentrics.z;

	// World normal
	const vec3 vnormal = normalize(mixBary(nrm0, nrm1, nrm2, barycentrics));
	vec3 normal    = normalize(vec3(vnormal * gl_WorldToObjectEXT));
	vec3 tangent = normalize(vec3(normalize(mixBary(tng0.xyz, tng1.xyz, tng2.xyz, barycentrics)) * gl_WorldToObjectEXT));
	vec3 bitangent = cross(normal, tangent);

	// Adjust world normal if ray hit a back facing triangle
	vec3 geonrm = normalize(vec3(normalize(cross(pos1 - pos0, pos2 - pos0)) * gl_WorldToObjectEXT));
	if(dot(geonrm, -gl_WorldRayDirectionEXT) < 0) { 
		geonrm = -geonrm;
	}
	if(dot(geonrm, normal) < 0) {
		normal    = -normal;
		tangent   = -tangent;
		bitangent = -bitangent;
	}

	// World position	
	//const vec3 worldPos = (modelMatrix * vec4(pos0 * barycentrics.x + pos1 * barycentrics.y + pos2 * barycentrics.z, 1.0)).xyz;
	const vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;



	
	vec3 combinedAttenuation = vec3(0);







	// Get a Light

	for (int i = 0; i < 8; i++) {

		Light light = lights.data[i];

		// Light
		float lightStrength = 1.2;
		float lightRadius = 5.6;
		vec3 lightPosition = vec3(3.2, 2.2, 3.5);

	
		lightRadius = light.radius;
		lightStrength = light.strength;
		lightPosition = vec3(light.posX, light.posY, light.posZ);

		vec3 lightDir = normalize(lightPosition - worldPos); 
		float lightAttenuation =  smoothstep(lightRadius, 0, length(lightPosition - worldPos));
		float irradiance = max(dot(lightDir, normal), 0.0) ;
		irradiance *= lightAttenuation * lightStrength;	

		// Shadow ray
		vec3 origin = worldPos + (-gl_WorldRayDirectionEXT * 0.001);
		float shadowFactor = 1;
		float minRayDist = 0.00001;
		float distanceToLight = distance(lightPosition.xyz, origin);
		uint  flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsCullFrontFacingTrianglesEXT;     	
		isShadowed = true;
		if (distanceToLight < lightRadius) {
			traceRayEXT(topLevelAS,	flags, 0xFF, 0, 0, 1, origin, minRayDist, lightDir, distanceToLight, 1);		
			if(isShadowed) {
				shadowFactor = 0;
			}
		}

		combinedAttenuation += vec3(lightAttenuation) *  shadowFactor * irradiance;

	}

	// Debug output
	//color = texture(sampler2D(textures[198], samp), texCoord).rgb;
	//color = normal;
	//color = vec3(texCoord, 0);
	//color = worldPos;



	

	rayPayload.color = combinedAttenuation;// * worldPos;
	rayPayload.hitPos = worldPos;
	rayPayload.normal = normal;

	rayPayload.color = vec3(0, 1, 0);

}*/