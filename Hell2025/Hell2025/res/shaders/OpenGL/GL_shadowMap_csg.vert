#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 shadowMatrices[6];
uniform int faceIndex;

out vec3 FragPos;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = shadowMatrices[faceIndex] * worldPos;
}