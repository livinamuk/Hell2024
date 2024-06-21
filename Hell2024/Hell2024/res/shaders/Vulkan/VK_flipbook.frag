#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

/*
layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 2, binding = 0) uniform sampler2D baseColorTexture;
layout(set = 2, binding = 1) uniform sampler2D normalTexture;
layout(set = 2, binding = 2) uniform sampler2D rmaTexture;
layout(set = 2, binding = 3) uniform sampler2D depthTexture;
layout(set = 2, binding = 4) uniform sampler2D raytracingOutput;
layout(set = 2, binding = 5) uniform sampler2D positionTexture;
layout(set = 2, binding = 7) uniform sampler2D glassTexture;*/

struct CameraData {
    mat4 projection;
    mat4 projectionInverse;
    mat4 view;
    mat4 viewInverse;
	float viewportWidth;
	float viewportHeight;
    float viewportOffsetX;
    float viewportOffsetY;
	float clipSpaceXMin;
    float clipSpaceXMax;
    float clipSpaceYMin;
    float clipSpaceYMax;
	float finalImageColorContrast;
    float finalImageColorR;
    float finalImageColorG;
    float finalImageColorB;
};

layout(set = 0, binding = 0) readonly buffer A {
    CameraData[4] data;
} cameraData;



struct MuzzleFlashData {
    mat4 modelMatrix;
    int frameIndex;
    int RowCount;
    int ColumnCount;
    float timeLerp;
};


layout(set = 0, binding = 11) readonly buffer MUZZLE_FLASH_DATA_BUFFER {
    MuzzleFlashData[4] data;
} muzzleFlashData;

layout( push_constant ) uniform constants {
	int playerIndex;
	int instanceOffset;
	int emptpy;
	int emptp2;
} PushConstants;


layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[1024];



void main() {

	int frameIndex =  muzzleFlashData.data[PushConstants.playerIndex].frameIndex;
	int rowCount =  muzzleFlashData.data[PushConstants.playerIndex].RowCount;
	int columnCount = muzzleFlashData.data[PushConstants.playerIndex].ColumnCount;
	float timeLerp =  muzzleFlashData.data[PushConstants.playerIndex].timeLerp;

	vec2 sizeTile =  vec2(1.0 / columnCount, 1.0 / rowCount);

	int frameIndex0 =  frameIndex - 1;
	int frameIndex1 = frameIndex0 + 1;

	vec2 tileOffset0 = ivec2(frameIndex0 % columnCount, frameIndex0 / columnCount) * sizeTile;
	vec2 tileOffset1 = ivec2(frameIndex1 % columnCount, frameIndex1 / columnCount) * sizeTile;

//	vec4 color0 = texture(u_MainTexture, tileOffset0 + Texcoord * sizeTile);//
//	vec4 color1 = texture(u_MainTexture, tileOffset1 + Texcoord * sizeTile);

	int textureIndex = PushConstants.emptp2;
	 vec4 color0 = texture(sampler2D(textures[textureIndex], samp), tileOffset0 + texCoord * sizeTile);
	 vec4 color1 = texture(sampler2D(textures[textureIndex], samp), tileOffset1 + texCoord * sizeTile);



	vec3 tint = vec3(1, 1, 1);
	vec4 color = mix(color0, color1, timeLerp) * vec4(tint, 1.0);
//	FragColor = color.rgba;



	// vec4 color0 = vec4(1,0,0,1);
	outFragColor = vec4(color);

}