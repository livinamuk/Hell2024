#version 400 core
layout (location = 0) in vec3 aPos;
layout (location = 5) in ivec4 aBoneID;
layout (location = 6) in vec4 aBoneWeight;

uniform mat4 model;
uniform bool isAnimated;
uniform mat4 skinningMats[64];

out vec3 FragPos;

void main()
{
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