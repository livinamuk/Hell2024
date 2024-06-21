#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

uniform mat4 projection;
uniform mat4 view;
uniform int playerIndex;
uniform int renderItemIndex;

//out vec3 Normal;
out vec2 TexCoords;
out flat int PlayerIndex;
out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;

out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

struct SkinnedRenderItem {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;

    int originalMeshIndex;
    int vertexBufferIndex;
    int baseColorTextureIndex;
    int normalTextureIndex;

    int rmaTextureIndex;
    int castShadow;
    int useEmissiveMask;
    int padding0;

    vec3 emissiveColor;
    int padding1;
};

layout(std430, binding = 17) readonly buffer renderItems {
    SkinnedRenderItem RenderItems[];
};

void main() {

	TexCoords = aTexCoord;
	PlayerIndex = playerIndex;

	mat4 model = RenderItems[renderItemIndex].modelMatrix;
	mat4 invereseModel = RenderItems[renderItemIndex].inverseModelMatrix;
	BaseColorTextureIndex =  RenderItems[renderItemIndex].baseColorTextureIndex;
	NormalTextureIndex =  RenderItems[renderItemIndex].normalTextureIndex;
	RMATextureIndex =  RenderItems[renderItemIndex].rmaTextureIndex;

	mat4 normalMatrix = transpose(invereseModel);
	attrNormal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	attrTangent = normalize((normalMatrix * vec4(aTangent, 0)).xyz);;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	gl_Position = projection * view * model * vec4(aPos, 1.0);
}