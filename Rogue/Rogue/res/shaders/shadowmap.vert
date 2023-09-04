#version 400 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 skinningMats[64];

out vec3 FragPos;

void main()
{
	vec4 worldPos = model * vec4(aPos, 1.0);
	gl_Position = worldPos;
}