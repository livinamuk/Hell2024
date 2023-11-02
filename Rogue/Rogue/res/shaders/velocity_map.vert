#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneID;
layout (location = 6) in vec4 aBoneWeight;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 WorldPos;

uniform bool isAnimated;
uniform mat4 skinningMats[85];
uniform mat4 skinningMatsPrevious[85];

out vec4 v_vCorrectedPosScreenSpace;
out vec4 v_vCurrentPosScreenSpace;
out vec4 v_vPreviousPosScreenSpace;

void main() {

	if (isAnimated) {	

		// Current pos
		vec4 totalLocalPos = vec4(0.0);			
		vec4 totalNormal = vec4(0.0);
		vec4 vertexPosition =  vec4(aPos, 1.0);
		vec4 vertexNormal = vec4(aNormal, 0.0);
		for(int i=0;i<4;i++)  {
			mat4 jointTransform = skinningMats[int(aBoneID[i])];
			vec4 posePosition =  jointTransform  * vertexPosition * aBoneWeight[i];	
			vec4 worldNormal = jointTransform * vertexNormal * aBoneWeight[i];			
			totalLocalPos += posePosition;		
			totalNormal += worldNormal;				
		}	
		WorldPos = (model * vec4(totalLocalPos.xyz, 1)).xyz;		

		// Previous pos
		vec4 totalLocalPosPrevious = vec4(0.0);
		for(int i=0;i<4;i++)  {
			mat4 jointTransform = skinningMatsPrevious[int(aBoneID[i])];
			vec4 posePosition =  jointTransform  * vertexPosition * aBoneWeight[i];
			totalLocalPosPrevious += posePosition;		
		}
		vec3 WorldPosPrevious = (model * vec4(totalLocalPosPrevious.xyz, 1)).xyz;




		vec4 vCurrPosWorldSpace = vec4(WorldPos, 1.0);//u_mCurrentModelMat  * a_vPosition;
		vec4 vPrevPosWorldSpace = vec4(WorldPosPrevious, 1.0);//u_mPreviousModelMat * a_vPosition;

		// DO THIS ON CPU
		mat4 u_mNormalMat = transpose(inverse(model));
		vec3 a_vNormal = totalNormal.xyz;
    
		vec3 vMotionVecWorldSpace =  vCurrPosWorldSpace.xyz - vPrevPosWorldSpace.xyz;
		vec3 vNormalVecWorldSpace = normalize(mat3(u_mNormalMat) * a_vNormal);

		float u_fStretchScale = 2;

		// Scaling the stretch.
		vec4 vStretchPosWorldSpace = vCurrPosWorldSpace;
		vStretchPosWorldSpace.xyz -= (vMotionVecWorldSpace * u_fStretchScale);

		v_vCurrentPosScreenSpace = projection * view * vCurrPosWorldSpace;
		v_vPreviousPosScreenSpace = projection * view * vPrevPosWorldSpace;
		vec4 vStretchPosScreenSpace = projection * view * vStretchPosWorldSpace;

		v_vCorrectedPosScreenSpace = dot(vMotionVecWorldSpace, vNormalVecWorldSpace) > 0.0 ? v_vCurrentPosScreenSpace : vStretchPosScreenSpace;
		
		gl_Position = v_vCorrectedPosScreenSpace;
	}

	else {
		WorldPos = (model * vec4(aPos.x, aPos.y, aPos.z, 1.0)).xyz;
		gl_Position = projection * view * vec4(WorldPos, 1.0);		

		v_vCurrentPosScreenSpace = projection * view * vec4(WorldPos, 1);
		v_vPreviousPosScreenSpace = projection * view * vec4(WorldPos, 1);
	}
}