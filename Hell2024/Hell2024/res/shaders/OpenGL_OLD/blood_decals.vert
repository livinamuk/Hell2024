#version 430 core
layout (location = 0) in vec3 aPos;

layout(std430, binding = 0) readonly buffer matrices {
    mat4 Matrices[];
};

uniform mat4 projectionView;

void main() {
	mat4 model = Matrices[gl_InstanceID];
	gl_Position = projectionView * model * vec4(aPos, 1.0);
}


