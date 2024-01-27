#pragma once
#include "../Common.h"

#define WORLD_WIDTH MAP_WIDTH
#define WORLD_DEPTH MAP_DEPTH
#define  WORLD_GRID_SPACING 0.1f

namespace Floorplan {

	void Init();
	void Update(float deltaTime);
	void PrepareRenderFrame();
	glm::mat4 GetProjectionMatrix();	
	glm::mat4 GetViewMatrix();	
	void NextMode();
	void PreviousMode();
	bool WasForcedOpen();
	void ForcedOpen();

}