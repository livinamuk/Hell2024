#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;
layout (location = 4) in ivec4 vBoneID;
layout (location = 5) in vec4 vBoneWeight;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform mat4 inverseModel;
uniform int playerIndex;
uniform int baseColorIndex;
uniform int normalMapIndex;
uniform int rmaIndex;
uniform vec4 clippingPlane;

out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

out flat int PlayerIndex;
out vec2 TexCoord;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;
out vec4 WorldPos;

void main() {
	BaseColorTextureIndex = baseColorIndex;
	NormalTextureIndex =  normalMapIndex;
	RMATextureIndex =  rmaIndex;
    
    TexCoord = vTexCoord;
    PlayerIndex = playerIndex;
    WorldPos = model * vec4(vPos, 1.0);

	mat4 normalMatrix = transpose(inverseModel);

	Normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
	Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;
	BiTangent = normalize(cross(Normal, Tangent));

    gl_Position = projection * view * WorldPos;
    gl_ClipDistance[0] = dot(WorldPos, clippingPlane);
}