#version 460
layout (location = 0) in vec2 vPos;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
	  gl_Position = projection * view * model * vec4(vPos, 0.0, 1.0);
}