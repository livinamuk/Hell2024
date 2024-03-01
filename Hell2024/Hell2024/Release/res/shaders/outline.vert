#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float viewportWidth;
uniform float viewportHeight;
uniform	int offsetX;
uniform	int offsetY;

void main() {

	gl_Position = projection * view * model *vec4(aPos, 1.0);	
	float pixelWidth = 2.0 / viewportWidth;
	float pixelHeight = 2.0 / viewportHeight;
	gl_Position.x += pixelWidth * gl_Position.z * offsetX;	
	gl_Position.y += pixelHeight * gl_Position.z * offsetY;		

}