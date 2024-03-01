#version 420 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneID;
layout (location = 6) in vec4 aBoneWeight;

out vec2 TexCoordVOut;

out vec3 attrNormalVOut;
out vec3 attrTangentVOut;
out vec3 attrBiTangentVOut;

uniform mat4 model;

void main() {	

	TexCoordVOut = aTexCoord;

	mat4 normalMatrix = transpose(inverse(model));
	attrNormalVOut = normalize((normalMatrix * vec4(aNormal, 0)).xyz);
	attrTangentVOut = (model * vec4(aTangent, 0.0)).xyz;
	attrBiTangentVOut = normalize(cross(attrNormalVOut, attrTangentVOut));

	gl_Position = model * vec4(aPos, 1.0);
}