#version 460 core

layout (location = 0) out vec4 FragOut;

uniform vec3 color;
in vec3 Normal;

void main() {
    FragOut.rgb = color;
    FragOut.rgb = Normal * 0.5 + 0.5;
	FragOut.a = 0.2;
	//FragOut.a = 1;
}
