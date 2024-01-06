#version 150

in vec2 position;
out vec2 blurTextureCoords[11];

uniform float targetWidth;

void main(void){

	gl_Position = vec4(position, 0.0, 1.0);
	vec2 centerTexCoords = position * 0.5 + 0.5;

	float pixelSize = 1.0 / targetWidth;

	for (int i=-5; i<=5; i++)
		blurTextureCoords[i+5] = centerTexCoords + vec2(pixelSize * i, 0.0);
}