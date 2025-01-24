#version 420 core

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec3 NormalOut;
layout (location = 2) out vec3 RMAOut;

in vec3 Normal;

void main() {
	
	BaseColorOut = vec4(0.5, 0.0, 0.0, 1.0); 
    NormalOut =  normalize(Normal);
	RMAOut.rgb = vec3(0.015 , 0.54, 1);
}