#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) out vec4 BaseColorOut;
layout (location = 2) out vec4 RMAOut;

readonly restrict layout(std430, binding = 0) buffer textureSamplerers {
	uvec2 textureSamplers[];
};

in flat int BaseColorTextureIndex;
in vec2 TexCoord;

void main() {

    vec4 baseColor = texture(sampler2D(textureSamplers[BaseColorTextureIndex]), TexCoord);

    if (baseColor.a < 0.5) {
        discard;
    }

    BaseColorOut.r = 0.5;
	BaseColorOut.g = 0;
    BaseColorOut.b = 0;
    BaseColorOut.a = baseColor.a;

//	BaseColorOut.rgb = vec3(1,0,0);

	// Can't decide if it looks better or worse with this.
	// It's barely noticable and is a waste of computation but still...
    // BaseColorOut.r = 0.5 - ((1 - (baseColor.a * baseColor.a)) * 0.1);

	RMAOut.rgb = vec3(0.015 , 0.6, 1);
}