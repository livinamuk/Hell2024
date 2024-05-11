#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;
out vec2 TexCoord;
out vec3 Normal;

out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;

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

layout(std430, binding = 13) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

void main() {

	TexCoord = aTexCoord;
	
	mat4 model = RenderItems[gl_InstanceID + gl_BaseInstance].modelMatrix;
	mat4 invereseModel = RenderItems[gl_InstanceID + gl_BaseInstance].inverseModelMatrix;
	BaseColorTextureIndex =  RenderItems[gl_InstanceID+ gl_BaseInstance].baseColorTextureIndex;
	NormalTextureIndex =  RenderItems[gl_InstanceID+ gl_BaseInstance].normalTextureIndex;
	RMATextureIndex =  RenderItems[gl_InstanceID+ gl_BaseInstance].rmaTextureIndex;

	mat4 normalMatrix = transpose(invereseModel);
	Normal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);

	gl_Position = projection * view * model *vec4(aPos, 1.0);
}

