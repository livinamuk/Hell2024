#version 420 core

layout (location = 0) out vec4 gAlbedo;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec3 gRMA;

in vec3 v_WorldNormal;
uniform mat4 u_MatrixWorld;
uniform float  u_Time;


void main() 
{
	if (u_Time > 0.9)
		discard;

	vec3 bloodColor = vec3(0.2, 0, 0); //  vec4(0.31, 0, 0, 1);
	gAlbedo.rgba = vec4(bloodColor, 1);
    gNormal =  normalize(v_WorldNormal);
    gRMA = vec3(0.15, 0.85, 0);
    gRMA = vec3(0.65, 0.0, 1);
}