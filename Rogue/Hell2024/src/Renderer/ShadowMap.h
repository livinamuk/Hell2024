#pragma once
#include <vector>
//#include <glm/glm.hpp>
#include "../Common.h"

#define SHADOW_MAP_SIZE 2048
#define SHADOW_NEAR_PLANE 0.1f
#define SHADOW_FAR_PLANE 20.0f	// change this to be the lights radius

class ShadowMap
{
public: // Methods
	void Init(); 
	void CleanUp();
	void Clear();

public: // Fields
	unsigned int _ID = { 0 };
	unsigned int _depthTexture = { 0 };
	std::vector<glm::mat4> _projectionTransforms;
};
