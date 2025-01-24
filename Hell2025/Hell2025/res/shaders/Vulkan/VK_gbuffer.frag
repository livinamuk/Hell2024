#version 460
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) in vec3 normal;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in flat int BaseColorTextureIndex;
layout (location = 3) in flat int NormalTextureIndex;
layout (location = 4) in flat int RMATextureIndex;
layout (location = 5) in vec3 attrNormal;
layout (location = 6) in vec3 attrTangent;
layout (location = 7) in vec3 attrBiTangent;
layout (location = 8) in vec3 WorldPos;
layout (location = 9) in flat int playerIndex;
layout (location = 10) in flat int useEmissiveMask;
layout (location = 11) in flat vec3 emissiveColor;

layout (location = 0) out vec4 baseColorOut;
layout (location = 1) out vec4 normalOut;
layout (location = 2) out vec4 rmaOut;
layout (location = 3) out vec4 positionOut;
layout (location = 4) out vec4 emissiveOut;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[1024];

void main() {

    vec4 baseColor = texture(sampler2D(textures[BaseColorTextureIndex], samp), texCoord);
    vec3 normalMap = texture(sampler2D(textures[NormalTextureIndex], samp), texCoord).rgb;
    vec3 rma = texture(sampler2D(textures[RMATextureIndex], samp), texCoord).rgb;
	mat3 tbn = mat3(normalize(attrTangent), normalize(attrBiTangent), normalize(attrNormal));
	vec3 normal = normalize(tbn * (normalMap.rgb * 2.0 - 1.0));

	baseColorOut = vec4(baseColor.rgb, 1.0);
	normalOut = vec4(normal, float(playerIndex) * 0.25);
	rmaOut = vec4(rma, 1.0);
    positionOut = vec4(WorldPos, 1.0);

	emissiveOut = vec4(0,0,0,0);
	if (useEmissiveMask == 1) {
		emissiveOut.rgb = vec3(rma.b) * emissiveColor;
		emissiveOut.a = 1.0;
	}

//	emissiveOut = vec4(emissiveColor,0);

	if (baseColor.a < 0.05) {
		//discard;
	}

	//baseColorOut.rgb = WorldPos;
}