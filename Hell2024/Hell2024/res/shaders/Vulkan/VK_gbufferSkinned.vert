#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;
layout (location = 4) in ivec4 vBoneID;
layout (location = 5) in vec4 vBoneWeight;

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
	float viewportWidth;
	float viewportHeight;
	float padding0;
	float padding1;
} cameraData;

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

layout(std140,set = 0, binding = 6) readonly buffer A {RenderItem3D data[];} renderItems;
layout(std140,set = 0, binding = 7) readonly buffer B {mat4 data[];} animatedTransforms;

void main() {	

	mat4 proj = cameraData.projection;
	mat4 view = cameraData.view;		
	mat4 model = renderItems.data[gl_InstanceIndex].modelMatrix;
	BaseColorTextureIndex =  renderItems.data[gl_InstanceIndex].baseColorTextureIndex;
	NormalTextureIndex =  renderItems.data[gl_InstanceIndex].normalTextureIndex;
	RMATextureIndex =  renderItems.data[gl_InstanceIndex].rmaTextureIndex;
	const int animatedTransformsOffset = renderItems.data[gl_InstanceIndex].animatedTransformsOffset;

	mat4 normalMatrix = transpose(inverse(model));						// FIX THIS IMMEDIATELY AKA LATER
	attrNormal = normalize((normalMatrix * vec4(vNormal, 0)).xyz);
	attrTangent = (model * vec4(vTangent, 0.0)).xyz;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));

	
	texCoord = vTexCoord;







	vec4 totalLocalPos = vec4(0.0);
	vec4 totalNormal = vec4(0.0);
	vec4 totalTangent = vec4(0.0);
			
	vec4 vertexPosition =  vec4(vPosition, 1.0);
	vec4 vertexNormal = vec4(vNormal, 0.0);
	vec4 vertexTangent = vec4(vTangent, 0.0);

	for(int i=0;i<4;i++)  {

		mat4 jointTransform = animatedTransforms.data[int(vBoneID[i]) + animatedTransformsOffset];
		vec4 posePosition =  jointTransform  * vertexPosition * vBoneWeight[i];
			
		vec4 worldNormal = jointTransform * vertexNormal * vBoneWeight[i];
		vec4 worldTangent = jointTransform * vertexTangent * vBoneWeight[i];

		totalLocalPos += posePosition;		
		totalNormal += worldNormal;	
		totalTangent += worldTangent;
		
	}	
	vec3 WorldPos = (model * vec4(totalLocalPos.xyz, 1)).xyz;		
	attrNormal =  (model * vec4(normalize(totalNormal.xyz), 0)).xyz;
	attrTangent =  (model * vec4(normalize(totalTangent.xyz), 0)).xyz;
	attrBiTangent = normalize(cross(attrNormal,attrTangent));
	
	//gl_Position = projection * view * vec4(WorldPos, 1.0);
	gl_Position = proj * view * vec4(WorldPos, 1.0);
}
