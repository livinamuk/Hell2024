#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

struct CloudPoint {
    vec4 position;
    vec4 normal;
    vec4 color;
};

layout(std430, binding = 0) buffer input_layout {
    CloudPoint PointCloud[];
};

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
out vec3 Color;

void main() {
	Color = PointCloud[gl_VertexID].color.xyz;
	//Color = aNormal.xyz;
    vec3 position = PointCloud[gl_VertexID].position.xyz;
	gl_Position = projection * view * vec4(position, 1.0);
}