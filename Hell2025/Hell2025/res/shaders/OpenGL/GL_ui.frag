#version 460 core
#extension GL_ARB_bindless_texture : enable

out vec4 FragColor;

layout (location = 0) in vec2 texCoord;
layout (location = 1) in flat int textureIndex;
layout (location = 2) in flat float colorTintR;
layout (location = 3) in flat float colorTintG;
layout (location = 4) in flat float colorTintB;

readonly restrict layout(std430, binding = 0) buffer textureSamplerers { 
	uvec2 textureSamplers[]; 
};

void main() {

	vec4 textureColor = texture(sampler2D(textureSamplers[textureIndex]), texCoord);    
    FragColor = vec4(textureColor);
	FragColor.rgb *= vec3(colorTintR, colorTintG, colorTintB);
}