#pragma once
#include <vector>
#include <glm/glm.hpp>

#define SHADOW_MAP_SIZE 4096
#define SHADOW_NEAR_PLANE 0.1f
#define SHADOW_FAR_PLANE 20.0f	// change this to be the lights radius

class ShadowMap
{
public: // Methods
	void Init(); 
	void CleanUp();

public: // Fields
	unsigned int _ID;
	unsigned int _depthTexture;
	std::vector<glm::mat4> _projectionTransforms;
};
