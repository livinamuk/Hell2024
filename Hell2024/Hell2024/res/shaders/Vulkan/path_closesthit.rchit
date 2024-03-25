#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

struct RayPayload {
	vec3 color;
};

struct RenderItem3D {
    mat4 modelMatrix;
    int meshIndex;
    int baseColorTextureIndex;
    int normaTextureIndex;
    int rmaTextureIndex;
	int vertexOffset;
	int indexOffset;
	int padding0;
	int padding1;
};

struct Vertex {
	vec3 position;
    float pad0;
    vec3 normal;
    float pad1;
    vec2 texCoord;
    float pad2;
    float pad3;
    vec3 tangent;
    float pad4;
    vec3 bitangent;
    float pad5;
    vec4 weight;
    ivec4 boneID;
};

hitAttributeEXT vec2 attribs;

layout(location = 0) rayPayloadInEXT RayPayload rayPayload;
layout(location = 1) rayPayloadEXT bool isShadowed;

layout(set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;
layout(std140,set = 0, binding = 2) readonly buffer A {RenderItem3D data[];} tlasInstances;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[1024];

layout(set = 3, binding = 0) readonly buffer Vertices { Vertex v[]; } vertices;
layout(set = 3, binding = 1) readonly buffer Indices { uint i[]; } indices;

vec3 mixBary(vec3 a, vec3 b, vec3 c, vec3 barycentrics) {
	return (a * barycentrics.x + b * barycentrics.y + c * barycentrics.z);
}

void main() {

	// Get vertex data from hit
	RenderItem3D renderItem = tlasInstances.data[gl_InstanceID];
	mat4 modelMatrix = renderItem.modelMatrix;
	int vertexOffset = renderItem.vertexOffset;
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
	const vec3 worldPos = (modelMatrix * vec4(pos0 * barycentrics.x + pos1 * barycentrics.y + pos2 * barycentrics.z, 1.0)).xyz;

	// Light
	float lightStrength = 1.2;
	float lightRadius = 5.6;
	vec3 lightPosition = vec3(3.2, 2.2, 3.5);

	vec3 lightDir = normalize(lightPosition - worldPos); 
	float lightAttenuation =  smoothstep(lightRadius, 0, length(lightPosition - worldPos));
	float irradiance = max(dot(lightDir, normal), 0.0) ;
	irradiance *= lightAttenuation * lightStrength;	

	// Shadow ray
	vec3 origin = worldPos;
	float shadowFactor = 1;
	float minRayDist = 0.001;
	float distanceToLight = distance(lightPosition.xyz, origin);
	uint  flags  = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsCullFrontFacingTrianglesEXT;     	
	isShadowed  = true;
	if (distanceToLight < lightRadius) {
		traceRayEXT(topLevelAS,	flags, 0xFF, 0, 0, 1, origin, minRayDist, lightDir, distanceToLight, 1);		
		if(isShadowed) {
			shadowFactor = 0;
		}
	}

	// Debug output
	vec3 color;
	color = texture(sampler2D(textures[198], samp), texCoord).rgb;
	color = normal;
	color = vec3(texCoord, 0);
	color = worldPos;
	rayPayload.color = color * shadowFactor * irradiance;

}