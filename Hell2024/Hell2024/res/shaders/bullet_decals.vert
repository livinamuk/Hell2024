#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;
out vec2 TexCoord;
out vec3 Normal;

layout(std430, binding = 0) readonly buffer decalMatrices {
    mat4 DecalMatrices[];
};

void main() {
	TexCoord = aTexCoord;
	Normal = aNormal;
	mat4 model = DecalMatrices[gl_InstanceID]; 
	gl_Position = projection * view * model *vec4(aPos, 1.0);
}

