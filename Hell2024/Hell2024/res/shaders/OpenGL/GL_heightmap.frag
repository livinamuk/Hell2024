#version 460 core
#extension GL_ARB_bindless_texture : enable

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;

layout (binding = 0) uniform sampler2D baseColorTexture;
layout (binding = 1) uniform sampler2D normapMapTexture;
layout (binding = 2) uniform sampler2D rmaTexture;
layout (binding = 3) uniform sampler2D MegaTexture;
layout (binding = 4) uniform sampler2D TreeMap;

in vec3 Normal;
in vec3 BiTangent;
in vec3 Tangent;
in vec2 TexCoords;
uniform int playerIndex;

in vec2 MegaTextureCoords;

void main() {
    vec3 baseColor = texture(baseColorTexture, TexCoords).rgb;
    vec3 normalMap = texture(normapMapTexture, TexCoords).rgb;
    vec3 rma = texture(rmaTexture, TexCoords).rgb;

    mat3 tbn = mat3(Tangent, BiTangent, Normal);
    vec3 normal = normalize(tbn * (normalMap * 2.0 - 1.0));

    BaseColorOut = vec4(baseColor, 1.0);
    NormalsOut = vec4(normal, float(playerIndex) * 0.25);
    RMAOut = vec4(rma, 1.0);

	//BaseColorOut.rgb = vec3(MegaTextureCoords, 0);
	
	//RMAOut.r = 0.0;
	//RMAOut.g = 0.0;
	//RMAOut.b = 0.0;
	
	float megaTextureValue = texture(MegaTexture, MegaTextureCoords).r;
	float treeValue = texture(TreeMap, MegaTextureCoords).r;

	if (megaTextureValue > 0) {
		BaseColorOut.r = 0.5;
		BaseColorOut.g = 0;
		BaseColorOut.b = 0;
		//RMAOut.rgb = vec3(0.015 , 0.6, 1);
		RMAOut.rgb = vec3(0.015 , 0.65, 1);
		//RMAOut.rgb = vec3(0.034 , 0.6, 1);
	}


	//BaseColorOut.rgb = vec3(treeValue);



//	BaseColorOut.rgb = vec3(1,0,0);

	// Can't decide if it looks better or worse with this.
	// It's barely noticable and is a waste of computation but still...
    // BaseColorOut.r = 0.5 - ((1 - (baseColor.a * baseColor.a)) * 0.1);


   // BaseColorOut.rgb = vec3(texture(MegaTexture, MegaTextureCoords).r, 0, 0);

	//BaseColorOut.rgb = vec3(1,0,0);

//	BaseColorOut.rgb = Normal;
}
