#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;

in flat int BaseColorTextureIndex;
in flat int NormalTextureIndex;
in flat int RMATextureIndex;
in vec3 attrNormal;
in vec3 attrTangent;
in vec3 attrBiTangent;

readonly restrict layout(std430, binding = 0) buffer textureSamplerers { 
	uvec2 textureSamplers[]; 
};

in vec2 TexCoord;

uniform int textureIndex;

void main() {

    vec4 baseColor = texture(sampler2D(textureSamplers[BaseColorTextureIndex]), TexCoord);    
    vec4 normalMap = texture(sampler2D(textureSamplers[NormalTextureIndex]), TexCoord);    
    vec4 rma = texture(sampler2D(textureSamplers[RMATextureIndex]), TexCoord);  

	mat3 tbn = mat3(normalize(attrTangent), normalize(attrBiTangent), normalize(attrNormal));	
	vec3 normal = normalize(tbn * (normalMap.rgb * 2.0 - 1.0));
	
    BaseColorOut = baseColor;
    NormalsOut.rgb = normal;
    RMAOut = rma;
}
