#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;
layout (location = 6) out vec4 EmssiveMask;

in flat int BaseColorTextureIndex;
in flat int NormalTextureIndex;
in flat int RMATextureIndex;
in flat int useEmissiveMask;
in flat int PlayerIndex;
in vec3 Normal;
in vec3 Tangent;
in vec3 BiTangent;
in vec3 emissiveColor;

readonly restrict layout(std430, binding = 0) buffer textureSamplerers {
	uvec2 textureSamplers[];
};

in vec2 TexCoord;

void main() {    
    vec4 baseColor = texture(sampler2D(textureSamplers[BaseColorTextureIndex]), TexCoord);    
    if (baseColor.a < 0.0025) {
        discard;
    }    
    vec3 normalMap = texture(sampler2D(textureSamplers[NormalTextureIndex]), TexCoord).rgb;   
    mat3 tbn = mat3(normalize(Tangent), normalize(BiTangent), normalize(Normal));
    normalMap = normalMap.rgb * 2.0 - 1.0;
    normalMap = normalize(normalMap);
    vec3 normal = normalize(tbn * (normalMap));
    vec4 rma = texture(sampler2D(textureSamplers[RMATextureIndex]), TexCoord);    
    BaseColorOut = baseColor;
    NormalsOut.rgb = normal;
    NormalsOut.a = float(PlayerIndex) * 0.25;
    RMAOut = rma;
    if (useEmissiveMask == 1) {
        EmssiveMask.rgb = vec3(rma.b) * emissiveColor;
        EmssiveMask.a = 1.0;
    }
}
