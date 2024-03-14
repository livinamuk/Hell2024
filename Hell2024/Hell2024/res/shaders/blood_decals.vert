#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;


layout(std430, binding = 0) readonly buffer decalMatrices {
    mat4 DecalMatrices[];
};


uniform mat4 pv;
//uniform mat4 model;

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;
out mat3 TBN;

out vec3 attrNormal;
out vec3 attrTangent;
out vec3 attrBiTangent;

out vec3 worldPosition;

void main()
{
	TexCoords = aTexCoord;	
	mat4 model = DecalMatrices[gl_InstanceID]; 
	gl_Position = pv * model * vec4(aPos, 1.0);
}


