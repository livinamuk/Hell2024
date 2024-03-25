#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 2, binding = 0) uniform sampler2D baseColor;
layout(set = 2, binding = 1) uniform sampler2D normals;
layout(set = 2, binding = 2) uniform sampler2D rma;
layout(set = 2, binding = 3) uniform sampler2D raytracingOutput;

void main() {

	vec3 baseColor = texture(baseColor, texCoord).rgb;
    vec3 raytracingOutputColor = texture(raytracingOutput, texCoord).rgb;

	outFragColor.rgb = raytracingOutputColor;
    outFragColor.a = 1;
}