#version 450

layout (location = 0) in vec3 vPosition;
layout (location = 2) in vec2 vTexCoord;

layout (location = 1) out vec2 texCoord[9];

//out vec2 blurTextureCoords[9];
//uniform float targetWidth;


layout( push_constant ) uniform PushConstants {
    int screenWidth;
    int screenHeight;
    int empty0;
    int empty1;
} pushConstants;


void main(void){

	gl_Position = vec4(vPosition, 1.0);
	//texCoord = vTexCoord;

	vec2 centerTexCoords = vPosition.xy * 0.5 + 0.5;
	float pixelSize = 1.0 / pushConstants.screenWidth;
	for (int i=-4; i<=4; i++) {
		texCoord[i+4] = centerTexCoords + vec2(pixelSize * i, 0.0);
	}
}