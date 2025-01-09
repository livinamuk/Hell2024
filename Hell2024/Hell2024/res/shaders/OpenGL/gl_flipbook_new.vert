#version 460 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
out vec2 TexCoord;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    TexCoord = vUV;
	gl_Position = projection * view * model * vec4(vPos, 1.0);
}