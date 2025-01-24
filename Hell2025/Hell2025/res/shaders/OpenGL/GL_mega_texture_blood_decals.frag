#version 460 core
#extension GL_ARB_bindless_texture : enable
layout (location = 0) out vec4 ColorOut;

in vec2 TexCoords;
uniform int textureIndex;

readonly restrict layout(std430, binding = 0) buffer textureSamplerers {
	uvec2 textureSamplers[];
};


//layout (binding = 0) uniform sampler2D decalTexture;

void main() {
	vec4 textureColor = texture(sampler2D(textureSamplers[textureIndex]), TexCoords);

	//vec4 textureColor = texture(decalTexture, TexCoords);

	if (textureColor.a > 0) {
		ColorOut.r = 1;//textureColor.a;
	} else {
		ColorOut.r = 0;
	}
		ColorOut.r = textureColor.a;
}
