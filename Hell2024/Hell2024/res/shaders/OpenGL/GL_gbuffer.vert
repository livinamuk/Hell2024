#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in ivec4 aBoneID;
layout (location = 5) in vec4 aBoneWeight;

uniform mat4 projection;
uniform mat4 view;

out vec2 TexCoord;
out flat int BaseColorTextureIndex;
out flat int NormalTextureIndex;
out flat int RMATextureIndex;
out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;

struct RenderItem3D {
    mat4 modelMatrix;
    int meshIndex;
    int baseColorTextureIndex;
    int normalTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int animatedTransformsOffset;
    int padding1;
};

layout(std430, binding = 1) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

void main() {

	TexCoord = aTexCoord;	
	mat4 model = RenderItems[gl_DrawID].modelMatrix;
	BaseColorTextureIndex =  RenderItems[gl_DrawID].baseColorTextureIndex;
	NormalTextureIndex =  RenderItems[gl_DrawID].normalTextureIndex;
	RMATextureIndex =  RenderItems[gl_DrawID].rmaTextureIndex;

	mat4 normalMatrix = transpose(inverse(model));						// FIX THIS IMMEDIATELY AKA LATER
	attrNormal = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	attrTangent = (model * vec4(aTangent, 0.0)).xyz;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	gl_Position = projection * view * model * vec4(aPos, 1.0);

}