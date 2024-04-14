#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 color;
layout (location = 0) out vec4 ColorOut;

void main() {
	ColorOut = vec4(color, 1.0);
}