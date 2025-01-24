#version 460

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

layout (location = 0) out vec2 texCoord;
layout (location = 1) out flat int textureIndex;
layout (location = 2) out flat float colorTintR;
layout (location = 3) out flat float colorTintG;
layout (location = 4) out flat float colorTintB;

struct RenderItem2D {
    mat4 modelMatrix;
    float colorTintR;
    float colorTintG;
    float colorTintB;
    int textureIndex;
};

layout(std430, binding = 2) readonly buffer renderItems { RenderItem2D RenderItems[]; };

void main() {
	texCoord = vTexCoord;	
	textureIndex =  RenderItems[gl_InstanceID].textureIndex;
	colorTintR =  RenderItems[gl_InstanceID].colorTintR;
	colorTintG =  RenderItems[gl_InstanceID].colorTintG;
	colorTintB =  RenderItems[gl_InstanceID].colorTintB;
	gl_Position = RenderItems[gl_InstanceID].modelMatrix * vec4(vPos.xy, 0, 1.0);
}