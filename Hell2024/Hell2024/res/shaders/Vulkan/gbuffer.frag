#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;
//layout (location = 2) in flat int textureIndex;
//layout (location = 3) in vec3 color;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[1024];

void main() {
    outFragColor = texture(sampler2D(textures[189], samp), texCoord).rgba;
    outFragColor = texture(sampler2D(textures[198], samp), texCoord).rgba;
	outFragColor.rgb = vec3(texCoord, 0);
    outFragColor.a = 1;
}