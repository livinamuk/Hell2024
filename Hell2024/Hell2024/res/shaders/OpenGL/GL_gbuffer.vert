#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;
layout (location = 4) in ivec4 vBoneID;
layout (location = 5) in vec4 vBoneWeight;

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
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;
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
    TexCoord = vTexCoord;

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

    Normal = normalize((normalMatrix * vec4(vNormal, 0)).xyz);
    Tangent = (modelMatrix * vec4(vTangent, 0.0)).xyz;
    BiTangent = normalize(cross(vNormal, Tangent));

    // Compute the final position of the vertex in screen space
    gl_Position = projection * view * modelMatrix * vec4(vPos, 1.0);

    // Pass the player index to the fragment shader
    PlayerIndex = playerIndex;
}