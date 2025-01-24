#version 460 core
layout (location = 0) in vec3 aPos;

struct RenderItem3D {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    int meshIndex;
    int baseColorTextureIndex;
    int normalMapTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int castShadow;
    int useEmissiveMask;
    float emissiveColorR;
    float emissiveColorG;
    float emissiveColorB;
    float aabbMinX;
    float aabbMinY;
    float aabbMinZ;
    float aabbMaxX;
    float aabbMaxY;
    float aabbMaxZ;
    float padding0;
    float padding1;
    float padding2;
};

layout(std430, binding = 12) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

uniform mat4 shadowMatrices[6];
uniform int faceIndex;
uniform int baseInstance;

out vec3 FragPos;

void main() {
	mat4 model = RenderItems[gl_InstanceID + gl_BaseInstance + baseInstance].modelMatrix;
	vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = shadowMatrices[faceIndex] * worldPos;
}