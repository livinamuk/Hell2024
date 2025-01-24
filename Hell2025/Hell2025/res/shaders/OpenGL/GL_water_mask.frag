#version 460 core

layout (location = 0) out vec2 WorldPosXZOut;
layout (location = 1) out float MaskOut;

in vec4 WorldPos;

void main() {    
    WorldPosXZOut.r = WorldPos.x;
    WorldPosXZOut.g = WorldPos.z;
    MaskOut = 1;
}
