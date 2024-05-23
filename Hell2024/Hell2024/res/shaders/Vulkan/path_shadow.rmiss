#version 460
#extension GL_EXT_ray_tracing : enable

struct RayPayload {
	vec4 color;
	int lightIndex;
	int padding0;
	int padding1;
	int padding2;
};


layout(location = 0) rayPayloadInEXT RayPayload rayPayload;

void main() {

	rayPayload.color = vec4(1,1,1,1);

}
