#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 color;
uniform bool uniformColor;
uniform float viewportWidth;
uniform float viewportHeight;
uniform float cameraX;
uniform float cameraZ;

out vec3 Color;

void main() {

	if (uniformColor)
		Color = color;
	else
		Color = aColor;

	gl_Position = projection * vec4(aPos.x, -aPos.z, aPos.y -1, 1.0);

	float pixelWidth = 2.0 / viewportWidth;
	float pixelHeight = 2.0 / viewportHeight;
	

	pixelWidth *= -cameraX;
	pixelHeight *= -cameraZ;
   
	gl_Position.x += pixelWidth * gl_Position.w;
	gl_Position.y += pixelHeight * gl_Position.w;

}