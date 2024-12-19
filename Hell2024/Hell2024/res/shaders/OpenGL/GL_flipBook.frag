#version 430 core

out vec4 FragColor;
in vec2 Texcoord;
in vec3 FragPos;

uniform int playerIndex;

layout (binding = 0) uniform sampler2D u_MainTexture;

struct MuzzleFlashData {
    mat4 modelMatrix;
    int frameIndex;
    int RowCount;
    int ColumnCount;
    float timeLerp;
};

layout(std430, binding = 18) readonly buffer MuzzleFlashDataArray {
    MuzzleFlashData muzzleFlashDataArray[];
};

void main() {

	int frameIndex = muzzleFlashDataArray[playerIndex].frameIndex;
	int rowCount = muzzleFlashDataArray[playerIndex].RowCount;
	int columnCount = muzzleFlashDataArray[playerIndex].ColumnCount;
	float timeLerp = muzzleFlashDataArray[playerIndex].timeLerp;

	vec2 sizeTile =  vec2(1.0 / columnCount, 1.0 / rowCount);

	frameIndex = 1;

	int frameIndex0 =  frameIndex;
	int frameIndex1 = frameIndex0 + 1;

	vec2 tileOffset0 = ivec2(frameIndex0 % columnCount, frameIndex0 / columnCount) * sizeTile;
	vec2 tileOffset1 = ivec2(frameIndex1 % columnCount, frameIndex1 / columnCount) * sizeTile;

	vec4 color0 = texture(u_MainTexture, tileOffset0 + Texcoord * sizeTile);
	vec4 color1 = texture(u_MainTexture, tileOffset1 + Texcoord * sizeTile);

	vec3 tint = vec3(1, 1, 1);
	vec4 color = mix(color0, color1, timeLerp) * vec4(tint, 1.0);
	FragColor = color.rgba;

    if (FragPos.y < 2.4) {
     vec3 waterColor = vec3(0.8, 1.0, 0.9);
        FragColor.rgb *=  waterColor * waterColor;
    }

	//FragColor = vec4(1,0,0,1);
}