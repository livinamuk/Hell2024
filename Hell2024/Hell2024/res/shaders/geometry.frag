#version 420 core

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalsOut;
layout (location = 2) out vec4 RMAOut;
layout (location = 3) out vec4 EmissiveOut;
layout (location = 4) out vec4 WorldSpacePositionOut;

layout (binding = 0) uniform sampler2D basecolorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;

in vec2 TexCoord;
in vec3 WorldPos;

uniform vec3 viewPos;

uniform mat4 view;

in vec3 attrNormal;
in vec3 attrTangent;
in vec3 attrBiTangent;

uniform float time;
uniform float screenWidth;
uniform float screenHeight;
uniform bool writeEmissive;
uniform bool sampleEmissive;
uniform vec3 lightColor;

uniform float projectionMatrixIndex;

void main()
{
    vec4 baseColor = texture(basecolorTexture, TexCoord);
    vec3 normalMap = texture2D(normalTexture, TexCoord).rgb;
    vec3 rma =  texture2D(rmaTexture, TexCoord).rgb;
   	mat3 tbn = mat3(normalize(attrTangent), normalize(attrBiTangent), normalize(attrNormal));	
	vec3 normal = normalize(tbn * (normalMap.rgb * 2.0 - 1.0));

    BaseColorOut = baseColor;
    NormalsOut = vec4(normal, 1.0);;
    RMAOut = vec4(rma, projectionMatrixIndex);
    
    
    if (baseColor.a < 0.1) {
        discard;
    }

	if (sampleEmissive) {
		if (rma.b > 0.1) {
			//EmissiveOut = vec4(lightColor * rma.b * 1.25 * vec3(1, 0.95, 0.95), 0);
			EmissiveOut = vec4(lightColor * rma.b, 0);
		} 
		else {
			EmissiveOut = vec4(0, 0, 0, 1);
		}
	} 
	else {
		EmissiveOut = vec4(0, 0, 0, 1);
	}
	
	if (!writeEmissive) {
		EmissiveOut.rgb *= 0.75;
		EmissiveOut.a = 0.75;
		
    BaseColorOut.a = 0.5;
	}
	

    WorldSpacePositionOut = vec4(WorldPos, 1.0);
   // RMAOut.rgb = vec3(1,0,0);
   // RMAOut.a = 1.0;

    
   //BaseColorOut.rgb = vec3(1,0,0);
}
