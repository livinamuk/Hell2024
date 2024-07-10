#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

uniform mat4 projection;
uniform mat4 view;
uniform int playerIndex;
uniform mat4 model;
uniform mat4 inverseModel;

out vec2 TexCoord;
out flat int PlayerIndex;
out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;

out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

struct CSGInstance {
    int baseColorTextureIndex;
    int normalTextureIndex;
    int rmaTextureIndex;
    int padding;
};

layout(std430, binding = 19) readonly buffer csgInstances {
    CSGInstance CSGInstances[];
};

void main() {

	TexCoord = aTexCoord;
	//mat4 model = RenderItems[gl_InstanceID + gl_BaseInstance].modelMatrix;
	//mat4 invereseModel = RenderItems[gl_InstanceID + gl_BaseInstance].inverseModelMatrix;
	BaseColorTextureIndex =  CSGInstances[gl_InstanceID+ gl_BaseInstance].baseColorTextureIndex;
	NormalTextureIndex =  CSGInstances[gl_InstanceID+ gl_BaseInstance].normalTextureIndex;
	RMATextureIndex =  CSGInstances[gl_InstanceID+ gl_BaseInstance].rmaTextureIndex;

	mat4 normalMatrix = transpose(inverseModel);
	attrNormal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	attrTangent = (model * vec4(aTangent, 0.0)).xyz;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	gl_Position = projection * view * model * vec4(aPos, 1.0);
	PlayerIndex = playerIndex;

}