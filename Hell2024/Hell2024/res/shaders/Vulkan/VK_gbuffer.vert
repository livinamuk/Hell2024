#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;

layout (location = 1) out vec2 texCoord;
layout (location = 2) out flat int BaseColorTextureIndex;
layout (location = 3) out flat int NormalTextureIndex;
layout (location = 4) out flat int RMATextureIndex;
layout (location = 5) out vec3 attrNormal;
layout (location = 6) out vec3 attrTangent;
layout (location = 7) out vec3 attrBiTangent;
layout (location = 8) out vec3 WorldPos;

layout(set = 0, binding = 0) readonly buffer CameraData {
    mat4 projection;
    mat4 projectionInverse;
    mat4 view;
    mat4 viewInverse;
} cameraData;

struct RenderItem3D {
    mat4 modelMatrix;
    int meshIndex;
    int baseColorTextureIndex;
    int normalTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int padding0;
    int padding1;
};


layout(std140,set = 0, binding = 2) readonly buffer A {RenderItem3D data[];} renderItems;

void main() {	

	mat4 proj = cameraData.projection;
	mat4 view = cameraData.view;		
	mat4 model = renderItems.data[gl_InstanceIndex].modelMatrix;
	BaseColorTextureIndex =  renderItems.data[gl_InstanceIndex].baseColorTextureIndex;
	NormalTextureIndex =  renderItems.data[gl_InstanceIndex].normalTextureIndex;
	RMATextureIndex =  renderItems.data[gl_InstanceIndex].rmaTextureIndex;

	mat4 normalMatrix = transpose(inverse(model));						// FIX THIS IMMEDIATELY AKA LATER
	attrNormal = normalize((normalMatrix * vec4(vNormal, 0)).xyz);
	attrTangent = (model * vec4(vTangent, 0.0)).xyz;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	WorldPos = (model * vec4(vPosition, 1.0)).xyz;

	texCoord = vTexCoord;
	gl_Position = proj * view * vec4(WorldPos, 1.0);
}
