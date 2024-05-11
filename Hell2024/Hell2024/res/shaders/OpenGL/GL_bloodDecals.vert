#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;
out vec2 TexCoord;
out flat int BaseColorTextureIndex;

struct RenderItem3D {
    mat4 modelMatrix;
    mat4 inverseModelMatrix; 
    int meshIndex;
    int baseColorTextureIndex;
    int normalTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int animatedTransformsOffset; 
    int castShadow;
    int useEmissiveMask;
    float emissiveColorR;
    float emissiveColorG;
    float emissiveColorB;
};

layout(std430, binding = 14) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

void main() {

	TexCoord = aTexCoord;
	mat4 model = RenderItems[gl_InstanceID + gl_BaseInstance].modelMatrix;
	BaseColorTextureIndex =  RenderItems[gl_InstanceID+ gl_BaseInstance].baseColorTextureIndex;
	gl_Position = projection * view * model *vec4(aPos, 1.0);
}

