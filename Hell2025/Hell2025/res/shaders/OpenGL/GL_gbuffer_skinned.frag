#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;
layout (location = 6) out vec4 EmssiveMask;

in vec2 TexCoords;
in flat int PlayerIndex;
in vec3 Normal;
in vec3 Tangent;
in vec3 BiTangent;
in flat int BaseColorTextureIndex;
in flat int NormalTextureIndex;
in flat int RMATextureIndex;

readonly restrict layout(std430, binding = 0) buffer textureSamplerers {
	uvec2 textureSamplers[];
};

void main() {

    vec4 baseColor = texture(sampler2D(textureSamplers[BaseColorTextureIndex]), TexCoords);
    vec4 normalMap = texture(sampler2D(textureSamplers[NormalTextureIndex]), TexCoords);
    vec4 rma = texture(sampler2D(textureSamplers[RMATextureIndex]), TexCoords);

    BaseColorOut.rgb = vec3(baseColor.xyz);
	BaseColorOut.a = 1.0;

//    normalMap = normalize(normalMap);
 
	mat3 tbn = mat3(normalize(Tangent), normalize(BiTangent), normalize(Normal));

    //mat3 tbn = mat3(Tangent, BiTangent, Normal);
   
   vec3 normal = normalize(tbn * (normalMap.rgb * 2.0 - 1.0));
   
    //normal = normalize(Normal);
    NormalsOut.rgb = vec3(normal);
  //  NormalsOut.rgb = vec3(attrNormal);
    //NormalsOut.rgb = mix(normal, attrNormal, 0.5);
	NormalsOut.a = float(PlayerIndex) * 0.25;


    RMAOut.rgb = rma.rgb;
	RMAOut.a = 1.0;

    EmssiveMask.rgb = vec3(0, 0, 0);
	EmssiveMask.a = 1.0;
    
	
    //RMAOut.r = 1.0;
    //RMAOut.g = 1.0;
    //EmssiveMask.rgb = vec3(0.5);
}
