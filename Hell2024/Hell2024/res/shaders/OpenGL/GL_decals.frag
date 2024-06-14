#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;

layout (binding = 0) uniform sampler2D Texture;

in vec3 Normal;
in vec2 TexCoord;

in flat int BaseColorTextureIndex;
in flat int NormalTextureIndex;
in flat int RMATextureIndex;

readonly restrict layout(std430, binding = 0) buffer textureSamplerers { 
	uvec2 textureSamplers[]; 
};

uniform int playerIndex;


void main() {

		
    vec4 baseColor = texture(sampler2D(textureSamplers[BaseColorTextureIndex]), TexCoord);    
    vec4 normalMap = texture(sampler2D(textureSamplers[NormalTextureIndex]), TexCoord);    
    vec4 rma = texture(sampler2D(textureSamplers[RMATextureIndex]), TexCoord);  

    if (baseColor.a < 0.9) {
        discard;
    }
    
    float roughness = 0.95;
    float metallic = 0.1;

    BaseColorOut = vec4(baseColor.rgb, 1.0f);
    NormalsOut = vec4(Normal, float(playerIndex) * 0.25);
    RMAOut =  vec4(roughness, metallic, 1, 1.0);

	
	//BaseColorOut = vec4(1, 0, 0, 1);
	//NormalsOut = vec4(1, 0, 0, 1);
	//RMAOut = vec4(1, 0, 0, 1);
}