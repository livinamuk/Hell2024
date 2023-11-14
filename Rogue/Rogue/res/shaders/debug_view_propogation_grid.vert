#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 color;
uniform bool uniformColor;

out vec3 Color;

#define MAP_WIDTH   32
#define MAP_HEIGHT  16
#define MAP_DEPTH   50
#define GRID_SPACING 0.2

flat out int x;
flat out int y;
flat out int z;

void main() {

	z = gl_InstanceID  % MAP_DEPTH;
	y = (gl_InstanceID  / MAP_DEPTH) % MAP_HEIGHT;
	x = gl_InstanceID  / (MAP_HEIGHT * MAP_DEPTH); 

	vec4 worldPos = model * vec4(aPos, 1.0);
	worldPos.xyz += vec3(x, y, z) * GRID_SPACING;

	gl_Position = projection * view * worldPos;

	Color = worldPos.xyz;
}