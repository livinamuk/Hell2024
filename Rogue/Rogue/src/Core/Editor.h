#pragma once
#include "../Common.h"

#define WORLD_WIDTH MAP_WIDTH
#define WORLD_DEPTH MAP_DEPTH
#define  WORLD_GRID_SPACING 0.1f

namespace Editor {

	void Init();
	void Update(float deltaTime);
	void PrepareRenderFrame();
	glm::mat4 GetProjectionMatrix();	
	glm::mat4 GetViewMatrix();	
	void NextMode();
	void PreviousMode();
	bool WasForcedOpen();
	void ForcedOpen();

	/*glm::vec3 GetEditorWorldPosFromCoord(int x, int z);
	int GetMouseGridX();
	int GetMouseGridZ();
	int GetCameraGridX();
	int GetCameraGridZ();
	int GetMouseScreenX();
	int GetMouseScreenZ();
	float GetMouseWorldX();
	float GetMouseWorldZ();

	bool CooridnateIsWall(int gridX, int gridZ);*/

}