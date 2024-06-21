#version 460 core
#extension GL_KHR_vulkan_glsl : enable

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;

layout (location = 0) in vec2 TexCoord;
layout (location = 1) in vec3 Normal;
layout (location = 2) in flat int BaseColorTextureIndex;
layout (location = 3) in flat int NormalTextureIndex;
layout (location = 4) in flat int RMATextureIndex;
layout (location = 5) in flat int playerIndex;

layout(set = 1, binding = 0) uniform sampler samp;
layout(set = 1, binding = 1) uniform texture2D textures[1024];

void main() {
		
	vec4 baseColor = texture(sampler2D(textures[BaseColorTextureIndex], samp), TexCoord);
	vec3 normalMap = texture(sampler2D(textures[NormalTextureIndex], samp), TexCoord).rgb;
	vec3 rma = texture(sampler2D(textures[RMATextureIndex], samp), TexCoord).rgb;
	
    if (baseColor.a < 0.9) {
        discard;
    }
    
    float roughness = 0.95;
    float metallic = 0.1;

    BaseColorOut = vec4(baseColor.rgb, 1.0f);
    NormalsOut = vec4(Normal, float(playerIndex) * 0.25);
    RMAOut =  vec4(roughness, metallic, 1, 1.0);

	BaseColorOut.a = 1;
}