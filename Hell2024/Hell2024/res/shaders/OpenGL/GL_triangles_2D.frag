#version 460 core
#extension GL_ARB_bindless_texture : enable

out vec4 FragColor;

void main() {
	discard;
    FragColor = vec4(1, 0, 0, 1);
}