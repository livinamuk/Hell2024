#version 460 core

layout (location = 0) in vec3 vPosition;

uniform mat4 projectionView;
uniform mat4 model;
out vec4 WorldPos;

void main() {
    WorldPos = model * vec4(vPosition, 1.0);
	gl_Position = projectionView * WorldPos;
}