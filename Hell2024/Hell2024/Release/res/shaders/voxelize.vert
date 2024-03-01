#version 440 core
#extension GL_ARB_shader_viewport_layer_array : enable 
#extension GL_AMD_vertex_shader_viewport_index : enable 
#extension GL_NV_viewport_array2 : enable 


layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 color;
uniform bool uniformColor;

#define MAP_WIDTH 32
#define VOXEL_SIZE 0.2

out vec3 Color;

out vec4 worldPos;

void main() {

	if (uniformColor)
		Color = color;
	else
		Color = aColor;

	worldPos = model * vec4(aPos, 1.0);

	// x is backwards for some reason
	//worldPos.x = (MAP_WIDTH * VOXEL_SIZE) - worldPos.x;

	// Select layer
	gl_Layer = int(worldPos.z / VOXEL_SIZE);	

	gl_Position = projection * view * worldPos;

		//Color = vec3(1 - worldPos.x);



}