#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 Color;

struct LightVolumeData {
	float aabbMinX;
	float aabbMinY;
	float aabbMinZ;
	float padding;
	float aabbMaxX;
	float aabbMaxY;
	float aabbMaxZ;
	float padding2;
};


layout(std430, binding = 19) buffer Buffer {
    LightVolumeData lightVolumeData[];
};

uniform int lightIndex;

void main() {

	Color = aNormal;
	vec3 position = aPos;
	position.x = min(position.x, lightVolumeData[lightIndex].aabbMaxX);
	position.y = min(position.y, lightVolumeData[lightIndex].aabbMaxY);
	position.z = min(position.z, lightVolumeData[lightIndex].aabbMaxZ);
	position.x = max(position.x, lightVolumeData[lightIndex].aabbMinX);
	position.y = max(position.y, lightVolumeData[lightIndex].aabbMinY);
	position.z = max(position.z, lightVolumeData[lightIndex].aabbMinZ);
	gl_Position = projection * view * model * vec4(position, 1.0);

}