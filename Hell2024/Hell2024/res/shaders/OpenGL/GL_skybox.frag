#version 420 core

layout (location = 0) out vec4 FinalLighting;
layout (location = 1) out vec4 Normal;
layout (binding = 0) uniform samplerCube cubeMap;

in vec3 TexCoords;

uniform int playerIndex;
uniform vec3 skyboxTint;

void contrastAdjust( inout vec4 color, in float c) {
    float t = 0.5 - c * 0.5;
    color.rgb = color.rgb * c + t;
}

void main() {


    FinalLighting.rgb = texture(cubeMap, TexCoords).rgb;

	FinalLighting.rgb *= skyboxTint;


		Normal.rgb = vec3(0,0,0);
		Normal.a = float(playerIndex) * 0.25;

//	FragColor.rgb = vec3(0,0,0);

	vec3 desaturdatedColor = vec3(dot(vec3(0.200, 0.500, 0.100), FinalLighting.rgb));
	FinalLighting.rgb = mix(desaturdatedColor, FinalLighting.rgb, 0.25);

	contrastAdjust(FinalLighting, 1.125);
	FinalLighting.rgb *= vec3(0.5);
    FinalLighting.a = 1.0;
// 	FinalLighting.rgb *= vec3(0, 0, 0);
	FinalLighting.rgb *= vec3(0.5);
	//FinalLighting.rgb *= vec3(0.5);
	//FinalLighting.rgb *= vec3(0.0);
}
