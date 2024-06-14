#version 460 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_Texcoord;

uniform mat4 u_MatrixWorld;
uniform int playerIndex;

out vec3 Normal;
out vec2 Texcoord;
out vec3 FragPos;

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

struct MuzzleFlashData {
    mat4 modelMatrix;
    int frameIndex;
    int RowCount;
    int ColumnCont;
    float timeLerp;
};

layout(std430, binding = 16) readonly buffer CameraDataArray {
    CameraData cameraDataArray[];
};

layout(std430, binding = 17) readonly buffer MuzzleFlashDataArray {
    MuzzleFlashData muzzleFlashDataArray[];
};

void main() {

	Texcoord = a_Texcoord;
	mat4 projection = cameraDataArray[playerIndex].projection;
	mat4 view = cameraDataArray[playerIndex].view;

	mat4 model = muzzleFlashDataArray[0].modelMatrix;

	//model = model * u_MatrixWorld * inverse(u_MatrixWorld);
	 model = u_MatrixWorld;

	FragPos = vec4(model * vec4(a_Position.xyz, 1.0)).xyz;


	gl_Position = projection * view * vec4(FragPos, 1);

	// this is fucking slow. do it on CPU.
	mat3 normalMatrix = transpose(inverse(mat3(model)));
	Normal= normalize(normalMatrix * a_Normal);
}