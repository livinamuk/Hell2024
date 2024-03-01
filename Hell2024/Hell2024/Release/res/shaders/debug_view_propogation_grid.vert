#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 color;
uniform bool uniformColor;
uniform float propogationGridSpacing;

out vec3 Color;

uniform int propogationTextureWidth;
uniform int propogationTextureHeight;
uniform int propogationTextureDepth;

flat out int x;
flat out int y;
flat out int z;

void main() {

	z = gl_InstanceID  % propogationTextureDepth;
	y = (gl_InstanceID  / propogationTextureDepth) % propogationTextureHeight;
	x = gl_InstanceID  / (propogationTextureHeight * propogationTextureDepth); 

	vec4 worldPos = model * vec4(aPos, 1.0);
	worldPos.xyz += vec3(x, y, z) * propogationGridSpacing;

	gl_Position = projection * view * worldPos;

	Color = worldPos.xyz;
}