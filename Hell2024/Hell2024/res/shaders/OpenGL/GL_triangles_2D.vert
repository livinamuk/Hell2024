#version 460

layout (location = 0) in vec2 vPos;

void main() {
	  gl_Position = vec4(vPos, 0.0, 1.0);
}