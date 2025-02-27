#version 150
in vec2 position;
out vec2 blurTextureCoords[9];
uniform float targetHeight;

void main(void){
	gl_Position = vec4(position, 0.0, 1.0);
	vec2 centerTexCoords = position * 0.5 + 0.5;
	float pixelSize = 1.0 / targetHeight;
	for (int i=-4; i<=4; i++) {
		blurTextureCoords[i+4] = centerTexCoords + vec2(0.0, pixelSize * i);
	}
}