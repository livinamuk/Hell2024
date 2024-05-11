#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 5) in ivec4 aBoneID;
layout (location = 6) in vec4 aBoneWeight;

struct RenderItem3D {
    mat4 modelMatrix;
    mat4 inverseModelMatrix; 
    int meshIndex;
    int baseColorTextureIndex;
    int normalTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int animatedTransformsOffset; 
    int castShadow;
    int useEmissiveMask;
    float emissiveColorR;
    float emissiveColorG;
    float emissiveColorB;
};

layout(std430, binding = 12) readonly buffer renderItems {
    RenderItem3D RenderItems[];
};

uniform bool isAnimated;
uniform mat4 skinningMats[64];

out vec3 FragPos;

void main()
{
	mat4 model = RenderItems[gl_InstanceID + gl_BaseInstance].modelMatrix;

	vec4 worldPos;
	vec4 totalLocalPos = vec4(0.0);
	
	vec4 vertexPosition =  vec4(aPos, 1.0);

	// Animated
	if (isAnimated)
	{
		for(int i=0;i<4;i++) 
		{
			mat4 jointTransform = skinningMats[int(aBoneID[i])];
			vec4 posePosition =  jointTransform  * vertexPosition * aBoneWeight[i];

			totalLocalPos += posePosition;	
		}
		worldPos = model * totalLocalPos;
				gl_Position = worldPos;
	}
	else // Not animated
	{
		worldPos = model * vec4(aPos, 1.0);
		gl_Position = worldPos;
	}
	

}