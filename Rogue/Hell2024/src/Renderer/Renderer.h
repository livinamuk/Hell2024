#pragma once
#include "Texture3D.h"
#include "../Common.h"
#include "../Core/Player.h"

namespace Renderer {

	void InitMinimumToRenderLoadingFrame();
	void Init();
	void RenderLoadingFrame();
	void RenderFrame(Player* player);
	void RenderEditorFrame();
	void RenderDebugMenu();
	void RenderUI();
	void HotloadShaders();
	void WipeShadowMaps();

	void QueueLineForDrawing(Line line);
	void QueuePointForDrawing(Point point);
	void QueueTriangleForLineRendering(Triangle& triangle);
	void QueueTriangleForSolidRendering(Triangle& triangle);
	void QueueUIForRendering(std::string textureName, int screenX, int screenY, bool centered, glm::vec3 color);
	void QueueUIForRendering(UIRenderInfo renderInfo);

	void ToggleDrawingLights();
	void ToggleDrawingProbes();
	void ToggleDrawingLines();
	void ToggleDrawingRagdolls();
	void ToggleDebugText();

	int GetRenderWidth();
	int GetRenderHeight();
	float GetPointCloudSpacing();
	void NextMode();
	void PreviousMode();
	void NextDebugLineRenderMode();
	void RecreateFrameBuffers(int currentPlayer);
	void CreatePointCloudBuffer();
	void CreateTriangleWorldVertexBuffer();
	glm::mat4 GetProjectionMatrix(float depthOfField);

	//std::vector<int> UpdateDirtyPointCloudIndices();
	//std::vector<glm::uvec4> UpdateDirtyGridChunks();

	inline ViewportMode _viewportMode = FULLSCREEN;

	inline int _method = 1;
	inline bool _shadowMapsAreDirty = true;
	

}