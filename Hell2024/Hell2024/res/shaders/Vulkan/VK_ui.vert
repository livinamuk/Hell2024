
#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 2) in vec2 vTexCoord;

layout (location = 1) out vec2 texCoord;
layout (location = 2) out flat int textureIndex;
layout (location = 3) out flat vec3 color;

struct RenderItem2D {
    mat4 modelMatrix;
    float colorTintR;
    float colorTintG;
    float colorTintB;
    int textureIndex;
};

layout(std140,set = 0, binding = 1) readonly buffer A {RenderItem2D data[];} renderItems;

void main() {
	mat4 modelMatrix = renderItems.data[gl_InstanceIndex].modelMatrix;
	textureIndex = renderItems.data[gl_InstanceIndex].textureIndex;
	color = vec3(renderItems.data[gl_InstanceIndex].colorTintR, renderItems.data[gl_InstanceIndex].colorTintG, renderItems.data[gl_InstanceIndex].colorTintB);
	gl_Position = modelMatrix * vec4(vPosition, 1.0);
	texCoord = vec2(vTexCoord.x, vTexCoord.y);
}
