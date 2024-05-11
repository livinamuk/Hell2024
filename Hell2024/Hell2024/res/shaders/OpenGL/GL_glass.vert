#version 460 core


layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

uniform mat4 projection;
uniform mat4 view;

out vec3 WorldPos;
out vec2 TexCoords;
out vec3 Normal;

out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;

struct RenderItem3D {
    mat4 modelMatrix;
    mat4 inverseModelMatrix; 
    int meshIndex;
    int baseColorTextureIndex;
    int normalTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int animatedTransformsOffset; 
    int castShadow;
    int useEmissiveMask;
    float emissiveColorR;
    float emissiveColorG;
    float emissiveColorB;
};

layout(std430, binding = 11) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

void main() {
		
	TexCoords = aTexCoord;	
	mat4 model = RenderItems[gl_InstanceID + gl_BaseInstance].modelMatrix;
	mat4 invereseModel = RenderItems[gl_InstanceID + gl_BaseInstance].inverseModelMatrix;
	BaseColorTextureIndex =  RenderItems[gl_InstanceID+ gl_BaseInstance].baseColorTextureIndex;
	NormalTextureIndex =  RenderItems[gl_InstanceID+ gl_BaseInstance].normalTextureIndex;
	RMATextureIndex =  RenderItems[gl_InstanceID+ gl_BaseInstance].rmaTextureIndex;
	
	mat4 normalMatrix = transpose(invereseModel);
	attrNormal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	attrTangent = (model * vec4(aTangent, 0.0)).xyz;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));
	TexCoords = aTexCoord;
	WorldPos = vec3(model * vec4(aPos, 1.0));	
	gl_Position = projection * view * vec4(WorldPos, 1.0);

}