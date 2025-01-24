#version 460 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;
layout (location = 3) in vec3 vTangent;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec4 WorldPos;

void main() {    
    WorldPos = model * vec4(vPos, 1.0);
    gl_Position = projection * view * WorldPos;
}