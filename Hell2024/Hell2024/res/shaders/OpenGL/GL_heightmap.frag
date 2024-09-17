#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;

layout (binding = 0) uniform sampler2D baseColorTexture;

in vec3 Normal;
in vec2 TexCoords;
uniform int PlayerIndex;

void main() {


    vec4 baseColor = texture(baseColorTexture, TexCoords);
	
	BaseColorOut.rgb = vec3(baseColor.rgb);
	//BaseColorOut.rgb = vec3(TexCoords, 0);
	BaseColorOut.a = 1.0f;

	NormalsOut.rgb = Normal;
	NormalsOut.a = float(PlayerIndex) * 0.25;

	RMAOut.rgb = vec3(1,0,0);
	RMAOut.a = 1.0f;
	
}
