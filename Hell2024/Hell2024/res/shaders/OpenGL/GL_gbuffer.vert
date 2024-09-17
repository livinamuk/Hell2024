#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in ivec4 aBoneID;
layout (location = 5) in vec4 aBoneWeight;

uniform mat4 projection;
uniform mat4 view;
uniform int playerIndex;
uniform int goldBaseColorTextureIndex;
uniform int goldRMATextureIndex;
uniform int instanceDataOffset;

out vec2 TexCoord;
out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;
out flat int useEmissiveMask;
out flat int PlayerIndex;
out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;
out vec3 emissiveColor;

struct RenderItem3D {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    int meshIndex;
    int materialIndex;
    int vertexOffset;
    int indexOffset;
    int castShadow;
    int useEmissiveMask;
    int isGold;
    float emissiveColorR;
    float emissiveColorG;
    float emissiveColorB;
    float aabbMinX;
    float aabbMinY;
    float aabbMinZ;
    float aabbMaxX;
    float aabbMaxY;
    float aabbMaxZ;
};

struct Material {
	int baseColorTextureIndex;
	int normalTextureIndex;
	int rmaTextureIndex;
	int emissiveTextureIndex;
};

layout(std430, binding = 1) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

layout(std430, binding = 3) readonly buffer materials {
    Material Materials[];
};

void main() {

	int index = gl_InstanceID + gl_BaseInstance + instanceDataOffset;

    // Set the texture coordinates
    TexCoord = aTexCoord;

    // Load render item and material data
    RenderItem3D renderItem = RenderItems[index];
    Material material = Materials[renderItem.materialIndex];

    // Base textures
    BaseColorTextureIndex = material.baseColorTextureIndex;
    NormalTextureIndex = material.normalTextureIndex;
    RMATextureIndex = material.rmaTextureIndex;

    // Check if the item is gold early on
    if (renderItem.isGold == 1) {
        BaseColorTextureIndex = goldBaseColorTextureIndex;
        RMATextureIndex = goldRMATextureIndex;
    }

    // Emissive mask and color
    useEmissiveMask = renderItem.useEmissiveMask;
    emissiveColor = vec3(renderItem.emissiveColorR, renderItem.emissiveColorG, renderItem.emissiveColorB);

    // Compute model-space and normal-space transformations
    mat4 modelMatrix = renderItem.modelMatrix;
    mat4 normalMatrix = transpose(renderItem.inverseModelMatrix);

    attrNormal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
    attrTangent = (modelMatrix * vec4(aTangent, 0.0)).xyz;
    attrBiTangent = normalize(cross(attrNormal, attrTangent));

    // Compute the final position of the vertex in screen space
    gl_Position = projection * view * modelMatrix * vec4(aPos, 1.0);

    // Pass the player index to the fragment shader
    PlayerIndex = playerIndex;
}