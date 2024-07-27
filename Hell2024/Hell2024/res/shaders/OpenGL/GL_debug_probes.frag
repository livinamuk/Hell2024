#version 460 core

layout (location = 0) out vec4 FragOut;

layout(rgba32f, binding = 1) uniform image3D texture3D;

in vec3 Color;

in flat int probeIndexX;
in flat int probeIndexY;
in flat int probeIndexZ;

void main() {

	vec3 probeColor = imageLoad(texture3D, ivec3(probeIndexX,probeIndexY, probeIndexZ)).rgb;

	if (probeColor == vec3(0,0,0)) {
	//	discard;
	}
	//discard;

    FragOut.rgb = probeColor;
	FragOut.a = 1.0;
}
