#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_ray_query : enable

layout (location = 0) in vec3 color;
layout (location = 1) in vec3 WorldPos;
layout (location = 0) out vec4 ColorOut;

layout(set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;

void main() {
	ColorOut = vec4(WorldPos, 1.0);

	vec3 lightPosition = vec3(2,2,2);
	vec3 L = normalize(lightPosition - WorldPos);
	float dist = distance(lightPosition, WorldPos); 

	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, topLevelAS, gl_RayFlagsTerminateOnFirstHitEXT, 0xFF, WorldPos, 0.01, L, dist);

	// Traverse the acceleration structure and store information about the first intersection (if any)
	rayQueryProceedEXT(rayQuery);

	// If the intersection has hit a triangle, the fragment is shadowed
	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT ) {
		discard;
	}
}