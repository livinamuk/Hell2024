#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec2 a_Texcoord;

//layout(location = 0) uniform mat4 projection;
//layout(location = 1) uniform mat4 view;
//layout(location = 2) uniform mat4 u_MatrixWorld;
//layout(location = 4) uniform float u_Time;

layout (binding = 0) uniform sampler2D u_PosTex;
layout (binding = 1) uniform sampler2D u_NormTex;

readonly restrict layout(std430, binding = 0) buffer textureSamplerers {
	uvec2 textureSamplers[];
};

//uniform int positionTextureIndex;
//uniform int normalTextureIndex;

struct RenderItem3D {
    mat4 modelMatrix;
    mat4 inverseModelMatrix;
    int meshIndex;
    int baseColorTextureIndex;
    int normalMapTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int castShadow;
    int useEmissiveMask;
    float emissiveColorR;
    float emissiveColorG;
    float emissiveColorB;
    float aabbMinX;
    float aabbMinY;
    float aabbMinZ;
    float aabbMaxX;
    float aabbMaxY;
    float aabbMaxZ;
    float padding0;
    float padding1;
    float padding2;
};


layout(std430, binding = 15) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

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

layout(std430, binding = 16) readonly buffer CameraDataArray {
    CameraData cameraDataArray[];
};

out vec3 Normal;
out vec3 v_ViewDir;
out vec3 v_fragPos;

uniform int playerIndex;

float LinearToGammaSpaceExact (float value) {
    if (value <= 0.0F)
        return 0.0F;
    else if (value <= 0.0031308F)
        return 12.92F * value;
    else if (value < 1.0F)
        return 1.055F * pow(value, 0.4166667F) - 0.055F;
    else
        return pow(value, 0.45454545F);
}

vec3 LinearToGammaSpace (vec3 linRGB) {
   return vec3(LinearToGammaSpaceExact(linRGB.r), LinearToGammaSpaceExact(linRGB.g), LinearToGammaSpaceExact(linRGB.b));
}

void main() {

	mat4 projection = cameraDataArray[playerIndex].projection;
	mat4 view = cameraDataArray[playerIndex].view;

	mat4 model = RenderItems[gl_InstanceID + gl_BaseInstance].modelMatrix;
	mat4 invereseModel = RenderItems[gl_InstanceID + gl_BaseInstance].inverseModelMatrix;
	int positionTextureIndex =  RenderItems[gl_InstanceID + gl_BaseInstance].baseColorTextureIndex;
	int normalTextureIndex =  RenderItems[gl_InstanceID + gl_BaseInstance].normalMapTextureIndex;
		
	float u_Time = RenderItems[gl_InstanceID+ gl_BaseInstance].emissiveColorR;

    int u_NumOfFrames = 81;
    int u_Speed = 35;
    int u_BoundingMax = 144;
    int u_BoundingMin = 116;
    vec3 u_HeightOffset = vec3(-45.4, -26.17, 12.7);

    u_BoundingMax = 1;
    u_BoundingMin = -1;
    u_HeightOffset = vec3(0, 0, 0);

    float currentSpeed = 1.0f / (u_NumOfFrames / u_Speed);
    float timeInFrames = ((ceil(fract(-u_Time * currentSpeed) * u_NumOfFrames)) / u_NumOfFrames) + (1.0 / u_NumOfFrames);

    vec3 v = a_Position;
    vec2 uv = a_Texcoord;

    timeInFrames = 0.0;
	timeInFrames = u_Time;

	vec2 TexCoord = vec2(uv.x, (timeInFrames + uv.y));
	vec4 texturePos = textureLod(sampler2D(textureSamplers[positionTextureIndex]), TexCoord, 0);
	vec4 textureNorm = textureLod(sampler2D(textureSamplers[normalTextureIndex]), TexCoord, 0);

    Normal = textureNorm.xzy * 2.0 - 1.0;
	mat4 normalMatrix = transpose(invereseModel);
	Normal = normalize((normalMatrix * vec4(Normal, 0)).xyz);

    gl_Position =  projection * view * model * vec4(texturePos.xzy, 1.0);

}