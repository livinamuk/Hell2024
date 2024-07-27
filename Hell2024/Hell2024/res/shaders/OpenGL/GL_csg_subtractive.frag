#version 460 core

layout (location = 0) out vec4 FragOut;

uniform vec3 color;
uniform bool useUniformColor;

in vec3 Normal;

void main() {

    FragOut.rgb = Normal * 0.5 + 0.5;
	FragOut.a = 0.2;

	if (useUniformColor) {
		FragOut.rgb = color;
		FragOut.a = 1.0;
	}
}
