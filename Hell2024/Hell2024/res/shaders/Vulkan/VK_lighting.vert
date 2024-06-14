#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 2) in vec2 vTexCoord;

layout (location = 1) out vec2 texCoord;

void main() {
	texCoord = vec2(vTexCoord.x, vTexCoord.y);
	gl_Position = vec4(vPosition, 1.0);
}
